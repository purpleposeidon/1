// FIXME: Be a dynamic library?
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdarg.h>

#include <1.h>

bool l1_connect_stem(struct l1_atr *ret, int wrapped, int mode) {
    if (mode == O_RDONLY) { return false; }
    assert(mode == O_WRONLY || mode == O_RDWR);
    if (wrapped != STDOUT_FILENO) { return false; }
    if (!isatty(wrapped)) { return false; }
    char *stem_var = getenv(L1_STEM_VAR);
    if (!stem_var) { return false; }
    int stem_fd = atoi(stem_var);
    if (stem_fd < 0) { return false; }
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("l1_connect_stem(): pipe");
        abort();
    }
    int read_end = pipefd[0];
    int write_end = pipefd[1];
    ssize_t error;
    {
        // Thanks, man cmsg(3), you've been invaluably helpful for using this API!
        #define NUM_FD 1
        int myfds[NUM_FD] = { read_end };
        char iobuf[1] = { L1_STEM_IOV_PAYLOAD };
        struct iovec io = {
            .iov_base = iobuf,
            .iov_len = sizeof(iobuf)
        };
        struct msghdr msg = { 0 };
        msg.msg_iov = &io;
        msg.msg_iovlen = 1;

        union {
            char buf[CMSG_SPACE(sizeof(myfds))];
            struct cmsghdr align;
        } u;
        msg.msg_control = u.buf;
        msg.msg_controllen = sizeof(u.buf);

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int) * NUM_FD);
        int *fdptr = (int *) CMSG_DATA(cmsg); // Initialize the payload
        memcpy(fdptr, myfds, NUM_FD * sizeof(int));
        error = sendmsg(stem_fd, &msg, 0);
    }
    if (error == -1) {
        perror("l1_connect_stem(): sendmsg");
        abort();
    }
    close(read_end);
    ret->fd = write_end;
    ret->status = 1;
    return true;
}

struct l1_atr l1_connect(int wrapped, int mode) {
    struct l1_atr ret;
    if (l1_connect_stem(&ret, wrapped, mode)) {
        return ret;
    }
    int naked = ioctl(wrapped, L1_IOCTL_UNWRAP);
    if (naked < 0) {
        // Okay, it wasn't wrapped...
        if (errno == ENOTTY) {
            ret.fd = wrapped;
            ret.status = 0;
            /*if (getenv("FORCE_LIB1") != NULL) {
            } else {
                ret.fd = wrapped;
                ret.status = 0;
            }*/
        } else {
            perror("l1_connect failed");
            abort();
        }
    } else {
        ret.fd = naked;
        ret.status = 1;
    }
    return ret;
}

void l1_init() {
    static bool setup = false;
    if (setup) return;
    setup = true;
    l1_atrin = l1_connect(STDIN_FILENO, O_RDONLY);
    l1_atrout = l1_connect(STDOUT_FILENO, O_WRONLY);
}


struct l1_packet l1_packet_init(char type, char *msg, int len) {
    struct l1_packet ret = {
        .msg_len = len,
        .version = L1_P_VERSION,
        .flags = l1_std_visible(type) ? L1_PF_STD_VISIBLE : 0,
        .type = type,
        .msg = msg,
    };
    return ret;
}

struct l1_packet l1_packet_new(char type, const char *msg, size_t msg_len) {
    struct l1_packet ret = l1_packet_init(type, malloc(msg_len), msg_len);
    memcpy(&ret.msg, msg, msg_len);
    return ret;
}

struct l1_packet l1_packet_copy(struct l1_packet *p) {
    struct l1_packet ret = *p;
    ret.msg = memcpy(malloc(p->msg_len), p->msg, p->msg_len);
    return ret;
}
void l1_packet_free(struct l1_packet p) {
    free(p.msg);
}

bool l1_known_packet_type(char type) {
    switch (type) {
        // See other EACH_L1_HEADER_TYPE
        case L1_PUSH:
        case L1_POP:
        case L1_FIELD_NAME:
        case L1_FIELD_TYPE:
        case L1_FIELD_VALUE:
        case L1_FIELD_TEXT:
        case L1_FORMAT:
        case L1_COMMENT:
        case L1_LABEL:
            return true;
        default:
            return false;
    }
}
bool l1_std_visible(char type) {
    switch (type) {
        // See other EACH_L1_HEADER_TYPE
        case L1_PUSH:
        case L1_POP:
        case L1_FIELD_NAME:
        case L1_FIELD_TYPE:
        case L1_COMMENT:
        case L1_LABEL:
        case L1_FIELD_VALUE:
            return false;
        case L1_FIELD_TEXT:
        case L1_FORMAT:
            return true;
        default:
            return true;
    }
}

void l1_dump(struct l1_packet *p) {
    fprintf(stderr, "msg_len = %i\n", p->msg_len);
    fprintf(stderr, "version = %i\n", p->version);
    fprintf(stderr, "flags = %i\n", p->flags);
    fprintf(stderr, "type = %i\n", p->type);
    fprintf(stderr, "msg = ");
    if (p->msg == NULL) {
        fprintf(stderr, "NULL");
    } else {
        write(STDERR_FILENO, p->msg, p->msg_len);
    }
    fprintf(stderr, "\n");
}

ssize_t l1_write(struct l1_atr *atr, struct l1_packet *packet) {
    // FIXME: sendmmsg(2) may be worth looking into
    // FIXME: The packet should be split up if it is larger than PIPE_BUF.
    if (atr->status == 1) {
        int full_header_len = ((void*)&packet->msg) - (void*)packet;
        int buff_len = full_header_len + packet->msg_len;
        char *buff = malloc(buff_len);

        memcpy(&buff[0], packet, full_header_len);
        memcpy(&buff[full_header_len], packet->msg, packet->msg_len);

        char *alloc = buff;
        while (buff_len > 0) {
            int ret = write(atr->fd, buff, buff_len);
            if (ret < 0) {
                // Hopefully there isn't a partial write followed by an error. :|
                // Hopefully there isn't a partial write interlaced with somebody else's write. :|
                // Boy, partial writes sure are a lot of trouble! Maybe they should just error out.
                return ret;
            }
            buff_len -= ret;
            buff = &buff[ret];
        }
        free(alloc);
        return packet->msg_len;
    } else {
        if ((packet->flags & L1_PF_STD_VISIBLE) == 0) {
            return 0;
        }
        return write(atr->fd, packet->msg, packet->msg_len);
    }
}

struct l1_packet l1_vpacketf(char type, const char *fmt, va_list ap) {
    struct l1_packet ret = l1_packet_init(type, NULL, 0);
    size_t msg_len = 0;
    FILE *buff = open_memstream(&ret.msg, &msg_len);
    vfprintf(buff, fmt, ap);
    fclose(buff);
    if (msg_len > USHRT_MAX) {
        // No.
        fprintf(stderr, "l1_vpacketf: message too long\n");
        abort();
    }
    ret.msg_len = (short) msg_len;
    return ret;
}
struct l1_packet l1_packetf(char type, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    struct l1_packet ret = l1_vpacketf(type, fmt, ap);
    va_end(ap);
    return ret;
}

void l1_vfwritef(struct l1_atr *atr, char type, const char *fmt, va_list ap) {
    struct l1_packet p = l1_vpacketf(type, fmt, ap);
    l1_write(atr, &p);
    l1_packet_free(p);
}
void l1_fwritef(struct l1_atr *atr, char type, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    l1_vfwritef(atr, type, fmt, ap);
    va_end(ap);
}
void l1_printf(char type, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    l1_vfwritef(&l1_atrout, type, fmt, ap);
    va_end(ap);
}

char l1_packet_type(const struct l1_packet *p) { return p->type; }
char *l1_packet_msg(const struct l1_packet *p) { return p->msg; }

// See other EACH_L1_HEADER_TYPE
void l1_push         (l1_entity          entity) { l1_printf(L1_PUSH,          "%i", entity      ); }
void l1_pop          (l1_entity          entity) { l1_printf(L1_POP,           "%i", entity      ); }
void l1_field_name   (const char *   field_name) { l1_printf(L1_FIELD_NAME,    "%s", field_name  ); }
void l1_field_type   (const char *   field_type) { l1_printf(L1_FIELD_TYPE,    "%s", field_type  ); }
void l1_field_value  (const char *  field_value) { l1_printf(L1_FIELD_VALUE,   "%s", field_value ); }
void l1_field_text   (const char *   field_text) { l1_printf(L1_FIELD_TEXT,    "%s", field_text  ); }
void l1_format       (const char *   formatting) { l1_printf(L1_FORMAT,        "%s", formatting  ); }
void l1_comment      (const char *      comment) { l1_printf(L1_COMMENT,       "%s", comment     ); }




ssize_t l1_read(struct l1_atr *atr, struct l1_packet *dst) {
    memset(dst, 0, sizeof(struct l1_packet));
    if (atr->status == 0) {
        // FIXME: There's probably a nicer way to choose the size.
        int m = 1024;
        *dst = l1_packet_init(L1_SYNTHETIC, NULL, 0);
        dst->msg = malloc(m);
        ssize_t n = read(atr->fd, dst->msg, m);
        if (n < 0) goto error;
        dst->msg_len = n;
        dst->flags = L1_PF_SYNTHETIC | L1_PF_STD_VISIBLE;
        return n;
    }
    ssize_t n = read(atr->fd, dst, L1_P_HEADER_PRELUDE_LEN);
    if (n == 0) {
        dst->flags = L1_PF_SYNTHETIC | L1_PF_STD_VISIBLE;
        dst->type = L1_SYNTHETIC;
        return 0;
    }
    if (n != L1_P_HEADER_PRELUDE_LEN) goto error;
    if (dst->version != L1_P_VERSION) goto error;
    n = read(atr->fd, &dst->flags, L1_P_VERSION);
    if (n != L1_P_VERSION) goto error;
    dst->msg = malloc(dst->msg_len);
    n = read(atr->fd, dst->msg, dst->msg_len);
    if (n != dst->msg_len) goto error;
    return n;
error:
    // Hmm. I've been reading too much kernel code.
    free(dst->msg);
    dst->msg = NULL;
    dst->msg_len = 0;
    errno = EPROTO;
    return -1;
}

ssize_t l1_puts(char type, const char *msg) {
    struct l1_packet p = l1_packet_init(type, (char *)msg, strlen(msg));
    return l1_write(&l1_atrout, &p);
}

ssize_t l1_putchar(char type, char c) {
    struct l1_packet p = l1_packet_init(type, &c, 1);
    return l1_write(&l1_atrout, &p);
}

static ssize_t l1_print_filled(char type, char fill, int len) {
    struct l1_packet p = l1_packet_init(type, malloc(len), len);
    ssize_t r = l1_write(&l1_atrout, &p);
    l1_packet_free(p);
    return r;
}

ssize_t l1_print_pad(char type, int width, const char *msg) {
    int len = strlen(msg);
    if (len >= abs(width)) {
        return l1_puts(type, msg);
    }
    if (width > 0) {
        return l1_print_filled(L1_FORMAT, ' ', width - len)
            + l1_puts(type, msg);
    } else {
        return l1_puts(type, msg);
            + l1_print_filled(L1_FORMAT, ' ', -width - len);
    }
}

void l1_flush(struct l1_atr *atr) {
    // no-op; we have no buffering.
}

#ifdef __GLIBC__
#include <printf.h>

#ifndef L1_N_PCUTS
#define L1_N_PCUTS 20
#endif
static int pcuti = 0;
static long pcuts[L1_N_PCUTS];
static bool l1_printing = false;

// https://www.gnu.org/software/libc/manual/html_node/Customizing-Printf.html
static int packet_type_specifier(
        FILE *stream,
        const struct printf_info *info,
        const void *const *args
) {
    if (info->width == 1 && info->left && l1_printing) {
        pcuts[pcuti++] = ftell(stream);
        assert(pcuti < L1_N_PCUTS);
        return 0;
        // This return value is weird.
        // As far as printf is concerned, 0 makes sense.
        // But %1m is always followed by the packet type,
        // which isn't properly part of the output.
        // So actually a negative length would make sense!
        // But -1 is used to reject the format specifier.
        // It'd be nice if I could do `info->nbytes -= 1`.
        // (But it doesn't actually matter; l1_printp calculates the return value properly.)
    }
    // FIXME: L1_PHIDE
    return -1;
}
static int packet_type_arginfo(
        const struct printf_info *info,
        size_t n,
        int *argtypes,
        int *size
) {
    if (info->width == 1 && info->left && l1_printing) {
        return 0;
    }
    return -1;
}

ssize_t l1_printp_from(const char *file, unsigned int line, const char *fmt, ...) {
    {
        static bool setup = false;
        if (!setup) {
            setup = true;
            l1_init();
            register_printf_specifier('m', packet_type_specifier, packet_type_arginfo);
        }
    }
    if (strstr(fmt, L1_PFMT("")) != fmt) {
        // (or L1_PHIDE)
        fprintf(stderr, "l1_printp: format string must start with some L1_P*\n");
        abort();
    }
    size_t pp_size = 0;
    char *buf = NULL;
    pcuti = 0;
    FILE *f = open_memstream(&buf, &pp_size);

    va_list ap;
    va_start(ap, fmt);
    l1_printing = true;
    int ret = vfprintf(f, fmt, ap);
    l1_printing = false;
    pcuts[pcuti] = ftell(f);

    if (file != NULL && file[0]) {
        // 1) You've decide you don't want to include the link to the source code for some reason.
        // 2) You look at the source to see how to disable it.
        // 3) You use this knowledge to remove all the emitted line numbers, everywhere.
        // 4) You, in benefitting this, deny use of it to others.
        char buff[24];
        sprintf(buff, "%u", line);
        l1_field_name("meta:emitter_source_file");
        l1_field_value(file);
        l1_field_name("meta:emitter_line");
        l1_field_value(buff);
        // It's spammy, but it's also C.
        // How about a static variable?
    }

    fclose(f);

    ssize_t stdlen = 0;
    for (int i = 0; i < pcuti; i++) {
        long start = pcuts[i];
        char type = buf[start++];
        long end = pcuts[i + 1];
        long msg_len = end - start;
        struct l1_packet p = l1_packet_init(type, &buf[start], msg_len);
        ssize_t err = l1_write(&l1_atrout, &p);
        // FIXME: Single write call would be nice.
        // But it's annoying because we would have to re-merge the packets if atrout->status == 0.
        if (err < 0) {
            return err;
        }
        if (p.flags & L1_PF_STD_VISIBLE) {
            stdlen += msg_len;
        }
    }

    va_end(ap);
    free(buf);
    pcuti = 0;
    if (ret < 0) {
        return ret;
    } else {
        return stdlen;
    }
}
#endif

// This is useless.
// If you're using it, you're trying hard to line things up.
// But it can't handle wide characters, or multibyte characters, or combining characters....
/*
ssize_t l1_field(
        const char *name,
        const char *type,
        const char *value,
        const char *text,
        int width
) {
    if (name) {
        l1_puts(L1_FIELD_NAME, name);
    }
    if (type) {
        l1_puts(L1_FIELD_TYPE, type);
    }
    if (value) {
        l1_puts(L1_FIELD_VALUE, value);
    }
    if (text) {
        return l1_print_pad(L1_FIELD_TEXT, width, text);
    } else {
        return 0;
    }
}
*/

