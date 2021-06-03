#include <unistd.h>
#include <stdlib.h>


int main(void) {
    char *buf = malloc(60);
    read(0, buf, 6);
    return 0;
}
