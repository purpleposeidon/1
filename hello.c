#include <1.h>

int main() {
    l1_init();
    l1_printp(
            L1_PPUSH "0"
            L1_PFIELD_NAME "greeting"
            L1_PFIELD_TEXT "Hello, world!"
            L1_PPOP "0"
            L1_PFORMAT "\n");
    return 0;
}
