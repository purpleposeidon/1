#include <unistd.h>
#include <stdlib.h>
#include <1.h>
#include <sys/types.h>

int raw_dump() {
    while (1) {
        struct l1_packet msg;
        ssize_t err = l1_read(&l1_atrin, &msg);
        if (err <= 0) {
            l1_packet_free(msg);
            if (err == 0) break;
            return -1;
        }
        char pt = l1_packet_type(&msg);
        char *rest = l1_packet_msg(&msg);
        switch (pt) {
            case L1_PUSH:
            case L1_POP:
                fprintf(stderr, "\x1b[36m\x1b[1m%c\x1b[0m\x1b[36m", pt);
                break;
            case L1_FIELD_NAME:
            case L1_FIELD_TYPE:
                fprintf(stderr, "\x1b[35m\x1b[1m%c\x1b[0m\x1b[35m", pt);
                break;
            case L1_FIELD_VALUE:
            case L1_FIELD_TEXT:
                fprintf(stderr, "\x1b[32m\x1b[1m%c\x1b[0m\x1b[32m", pt);
                break;
            case L1_FORMAT:
            case L1_COMMENT:
                fprintf(stderr, "\x1b[34m\x1b[1m%c\x1b[0m\x1b[34m", pt);
                break;
            default:
                fprintf(stderr, "\x1b[31m\x1b[1m%c\x1b[0m\x1b[31m", pt);
                break;
        }
        fwrite(rest, 1, msg.msg_len, stderr);
        fprintf(stderr, "\x1b[0m");
        l1_packet_free(msg);
    }
    return 0;
}
int std_mark() {
    bool b = true;
    while (1) {
        struct l1_packet msg;
        ssize_t err = l1_read(&l1_atrin, &msg);
        if (err <= 0) {
            fprintf(stderr, "\n");
            l1_packet_free(msg);
            return -1;
        }
        char pt = l1_packet_type(&msg);
        char *rest = l1_packet_msg(&msg);
        if (l1_std_visible(pt)) {
            if (b) {
                fprintf(stderr, "\x1b[7m");
            }
            b ^= true;
            fwrite(rest, sizeof(char), msg.msg_len - 1, stderr);
            fprintf(stderr, "\x1b[0m");
        }
        l1_packet_free(msg);
    }
    return 0;
}
int main(int argc, char **argv) {
    l1_init();

    if (argc == 1) {
        return raw_dump();
    } else {
        return std_mark();
    }

    // FIXME: Writes ATROUT, not STDOUT.
}
