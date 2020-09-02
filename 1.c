#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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
            fprintf(stderr, "lib1: Can't handle protocol version %i\n", protocol_version);
        }
    }
}
void l1_init_atr() {
    l1_atrin = l1_connect_reader(STDIN_FILENO);
    l1_atrout = l1_connect_writer(STDOUT_FILENO);
}


bool std_visible(char t) {
    switch (t) {
        case L1_FIELD_VALUE:
        case L1_FIELD_TEXT:
        case L1_FORMAT:
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

ssize_t l1_write(struct l1_atr *atr, struct l1_packet *packet) {
    // FIXME: sendmmsg(2) may be worth looking into
    if (packet->len <= 1) {
        fprintf(stderr, "empty packet\n");
        abort();
    }
    if (atr->status == L1_ATR_STATUS_UNKNOWN) {
        fprintf(stderr, "l1_atr uninitialized\n");
        abort();
    } else if (atr->status == L1_ATR_STATUS_STD) {
        if (!std_visible(packet->msg[0])) { return 0; }
        return write(atr->fd, &packet->msg[1], packet->len);
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

void l1_push(l1_entity entity) {
    l1_printf(L1_PUSH, "%i", entity);
}
void l1_pop(l1_entity entity) {
    l1_printf(L1_POP, "%i", entity);
}
void l1_field_name(const char *field_name) {
    l1_printf(L1_FIELD_NAME, "%s", field_name);
}
void l1_field_type(const char *field_type) {
    l1_printf(L1_FIELD_TYPE, "%s", field_type);
}
void l1_field_value(const char *field_value) {
    l1_printf(L1_FIELD_VALUE, "%s", field_value);
}
void l1_field_text(const char *field_text) {
    l1_printf(L1_FIELD_TEXT, "%s", field_text);
}
void l1_format(const char *formatting) {
    l1_printf(L1_FORMAT, "%s", formatting);
}
void l1_comment(const char *comment) {
    l1_printf(L1_COMMENT, "%s", comment);
}




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
