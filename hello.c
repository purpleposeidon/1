#define _POSIX_C_SOURCE 200809L
#include <1.h>

int main() {
    l1_init();
    l1_push(0);
    l1_field_name("greeting");
    l1_field_text("Hello, world!");
    l1_format("\n");
    l1_pop(0);
    return 0;
}
