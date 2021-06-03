#include <1.h>

int main(int argc, char **argv) {
    if (argc == 2) {
        l1_printp(L1_PFORMAT "hi\n");
        return 0;
    }
    l1_printp(
            L1_PPUSH "0"
            L1_PFIELD_NAME "greeting"
            L1_PFIELD_TEXT "Hello, attributes!"
            L1_PFIELD_TEXT "...Wait a minute! What if rendevouz was done by passing a pipe as ancillary data over a unix socket? Does it get closed if it isn't accepted!? :O"
            // Does the kernel close it if the ancillary message is ignored????
            L1_PPOP "0"
            L1_PFORMAT "\n");
    return 0;
}
