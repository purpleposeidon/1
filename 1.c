#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <1.h>

static int protocol_version = -1;

// Only protocol 0 is implemented, so just make blind assumptions
struct l1_atr l1_connect_reader(int fd) {
    struct l1_atr ret = {
        .fd = fd,
        .protocol_version = protocol_version,
    };
    if (protocol_version == -1) {
        ret.status = L1_ATR_STATUS_STD;
    } else {
        {
            struct stat stat;
            int err = fstat(fd, &stat);
            if (err == -1) {
                perror("fstat");
                abort();
            }
            if ((stat.st_mode & S_IFMT) == S_IFSOCK) {
                ret.status = L1_ATR_STATUS_ATR;
            }
        }
    }
    return ret;
}
struct l1_atr l1_connect_writer(int fd) {
    return l1_connect_reader(fd);
}
struct l1_atr l1_conect_std(int fd) {
    struct l1_atr ret = {
        .fd = fd,
        .protocol_version = protocol_version,
        .status = L1_ATR_STATUS_STD,
    };
    return ret;
}

void l1_init() {
    l1_init_lib();
    l1_init_atr();
}
void l1_init_lib() {
    static bool l1_setup = false;
    if (l1_setup) {
        return;
    }
    l1_setup = true;
    {
        char *env_one = getenv("LIB1");
        while (env_one && env_one[0]) {
            if (env_one[0] == ';') {
                env_one = &env_one[1];
                continue;
            }
            // label...
            int n, parsed;
            char *dst;
            int number;
            n = sscanf(env_one, "%m[a-z_]:%i%n", &dst, &number, &parsed);

            if (n == 2 && strcmp(dst, "protocol_version") == 0) {
                protocol_version = number;
            }
            free(dst);
            env_one = &env_one[parsed];
        }
        if (protocol_version == -1) {
            // unset: act std
        } else if (protocol_version == 0) {
            // env var set: act atr
        } else {
            fprintf(stderr, "l1_init_lib: Can't handle protocol version %i\n", protocol_version);
        }
    }
}
void l1_init_atr() {
    l1_atrin = l1_connect_reader(STDIN_FILENO);
    l1_atrout = l1_connect_writer(STDOUT_FILENO);
}


bool l1_std_visible(char t) {
    switch (t) {
        // See other EACH_L1_HEADER_TYPE
        case L1_FIELD_TEXT:
        case L1_FORMAT:
        case L1_LABEL:
        case '?': // FIXME
            return true;
        default: return false;
    }
}

struct l1_packet l1_clone_packet(struct l1_packet *p) {
    struct l1_packet ret = {
        .msg = memcpy(malloc(p->len), p->msg, p->len),
        .len = p->len,
    };
    return ret;
}
void l1_drop_packet(struct l1_packet p) {
    free(p.msg);
}
bool l1_known_packet_type(char t) {
    switch (t) {
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
struct l1_packet l1_new_packet(char t, const char *msg, size_t len) {
    if (len == 0) {
        fprintf(stderr, "l1_new_packet: empty packet\n");
        abort();
    }
    char *buf = malloc(len + 1);
    buf[0] = t;
    memcpy(&buf[1], msg, len);
    struct l1_packet ret = {
        .msg = buf,
        .len = len + 1,
    };
    return ret;
}
struct l1_packet l1_new_hpacket(const char *msg, size_t len) {
    if (len == 0) {
        fprintf(stderr, "l1_new_hpacket: no packet header\n");
        abort();
    }
    if (len <= 1) {
        fprintf(stderr, "l1_new_hpacket: empty packet\n");
        abort();
    }
    if (!l1_known_packet_type(msg[0])) {
        fprintf(stderr, "l1_new_hpacket: unknown packet type\n");
        abort();
    }
    struct l1_packet ret = {
        .msg = (char *)msg,
        .len = len,
    };
    return ret;
}

ssize_t l1_write(struct l1_atr *atr, struct l1_packet *packet) {
    // FIXME: sendmmsg(2) may be worth looking into
    if (packet->len <= 1) {
        fprintf(stderr, "l1_write: empty packet\n");
        abort();
    }
    if (atr->status == L1_ATR_STATUS_UNKNOWN) {
        fprintf(stderr, "l1_write: l1_atr uninitialized\n");
        abort();
    } else if (atr->status == L1_ATR_STATUS_STD) {
        if (!l1_std_visible(packet->msg[0])) { return 0; }
        return write(atr->fd, &packet->msg[1], packet->len - 1);
    } else {
        return write(atr->fd, packet->msg, packet->len);
    }
}

struct l1_packet l1_vpacketf(char t, const char *fmt, va_list ap) {
    struct l1_packet ret;
    FILE *buff = open_memstream(&ret.msg, &ret.len);

    fprintf(buff, "%c", t);

    vfprintf(buff, fmt, ap);

    fclose(buff);
    return ret;
}
struct l1_packet l1_packetf(char t, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    struct l1_packet ret = l1_vpacketf(t, fmt, ap);
    va_end(ap);
    return ret;
}

void l1_vfwritef(struct l1_atr *atr, char t, const char *fmt, va_list ap) {
    struct l1_packet p = l1_vpacketf(t, fmt, ap);
    l1_write(atr, &p);
    l1_drop_packet(p);
}
void l1_fwritef(struct l1_atr *atr, char t, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    l1_vfwritef(atr, t, fmt, ap);
    va_end(ap);
}
void l1_printf(char t, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    l1_vfwritef(&l1_atrout, t, fmt, ap);
    va_end(ap);
}

char l1_packet_type(const struct l1_packet *p) { return p->msg[0]; }
char *l1_packet_msg(const struct l1_packet *p) { return &p->msg[1]; }

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
    dst->len = 0;
    dst->msg = NULL;
    char minibuff = 0;
    ssize_t n = recv(atr->fd, &minibuff, 1, MSG_PEEK | MSG_TRUNC);
    if (n <= 0) {
        return n;
    }
    void *msg = malloc(n);
    n = recv(atr->fd, msg, n, 0);
    if (n <= 0) {
        free(msg);
        return n;
    }
    dst->len = n;
    dst->msg = msg;
    return n;
}


ssize_t l1_putchar(char t, char c) {
    struct l1_packet p;
    p.len = 2;
    char msg[2] = {t, c};
    p.msg = &msg[0];
    return l1_write(&l1_atrout, &p);
}

ssize_t l1_puts(char t, const char *msg) {
    struct l1_packet p = l1_new_packet(t, msg, strlen(msg));
    // FIXME: iovec
    ssize_t ret = l1_write(&l1_atrout, &p);
    l1_drop_packet(p);
    return ret;
}

int l1_flush(struct l1_atr *atr) {
    // We have no buffering!
    return 0;
}

#ifdef __GLIBC__
#include <printf.h>

#ifndef N_PCUTS
#define N_PCUTS 20
#endif
static int pcuti = 0;
static long pcuts[N_PCUTS];

static int l1_print_packet(
        FILE *stream,
        const struct printf_info *info,
        const void *const *args
) {
    if (info->width == 1 && info->left == 1) {
        pcuts[pcuti++] = ftell(stream);
        assert(pcuti < N_PCUTS);
        return 0;
        // This return value is weird.
        // As far as printf is concerned, 0 makes sense.
        // But %1m is always followed by a 1-byte packet header,
        // which isn't properly part of the output.
        // So actually a negative length would make sense!
        // But -1 is used to reject the format specifier.
        // It'd be nice if I could do `info->nbytes -= 1`.
    }
    return -1;
}
static int l1_print_packet_arginfo(
        const struct printf_info *info,
        size_t n,
        int *argtypes,
        int *size
) {
    if (info->width == 1 && info->left == 1) {
        return 0;
    }
    return -1;
}

ssize_t l1_printp(const char *fmt, ...) {
    {
        static bool setup = false;
        if (!setup) {
            setup = true;
            register_printf_specifier('m', l1_print_packet, l1_print_packet_arginfo);
        }
    }
    if (strstr(fmt, L1_PFMT("")) != fmt) {
        fprintf(stderr, "l1_printp: format string must start with L1_P*\n");
        abort();
    }
    size_t pp_size = 0;
    char *buf = NULL;
    pcuti = 0;
    FILE *f = open_memstream(&buf, &pp_size);

    va_list ap;
    va_start(ap, fmt);
    int ret = vfprintf(f, fmt, ap);
    pcuts[pcuti] = ftell(f);
    fclose(f);

    ssize_t stdlen = 0;
    for (int i = 0; i < pcuti; i++) {
        long start = pcuts[i];
        long end = pcuts[i + 1];
        long len = end - start;
        struct l1_packet p = {
            .msg = &buf[start],
            .len = len,
        };
        ssize_t err = l1_write(&l1_atrout, &p);
        if (err < 0) {
            return err;
        }
        if (l1_std_visible(p.msg[0])) {
            stdlen += len - 1;
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

// FIXME: printf("%*s", width, thing) is pretty common, at least in ls.
/*
void l1_print_field(
        conar char *prefix,
        const char *name,
        const char *value, // nullable
        const char *text,
        int width,
        conar char *suffix,
) {
}
*/
