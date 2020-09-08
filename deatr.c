#include <unistd.h>
#include <stdlib.h>
#include <1.h>

int main(int argc, char **argv) {
    l1_init_lib();
    l1_atrin = l1_connect_reader(STDIN_FILENO);
    if (l1_atrin.status != L1_ATR_STATUS_ATR) {
        fprintf(stderr, "%s: stdin not atr pipe\n", argv[0]);
        return EXIT_FAILURE;
    }

    // FIXME: Writes ATROUT, not STDOUT.
    while (1) {
        struct l1_packet msg;
        ssize_t err = l1_read(&l1_atrin, &msg);
        if (err <= 0) {
            printf("\n");
            l1_drop_packet(msg);
            return -1;
        }
        char pt = l1_packet_type(&msg);
        char *rest = &msg.msg[1];
        switch (pt) {
            case L1_PUSH:
            case L1_POP:
                printf("\x1b[36m\x1b[1m%c\x1b[0m\x1b[36m", pt);
                break;
            case L1_FIELD_NAME:
            case L1_FIELD_TYPE:
                printf("\x1b[35m\x1b[1m%c\x1b[0m\x1b[35m", pt);
                break;
            case L1_FIELD_VALUE:
            case L1_FIELD_TEXT:
                printf("\x1b[32m\x1b[1m%c\x1b[0m\x1b[32m", pt);
                break;
            case L1_FORMAT:
            case L1_COMMENT:
                printf("\x1b[34m\x1b[1m%c\x1b[0m\x1b[34m", pt);
                break;
            default:
                printf("\x1b[31m\x1b[1m%c\x1b[0m\x1b[31m", pt);
                break;
        }
        fwrite(rest, sizeof(char), msg.len - 1, stdout);
        printf("\x1b[0m");
        l1_drop_packet(msg);
    }
}
