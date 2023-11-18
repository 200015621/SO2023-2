#include <stdio.h>
#include <time.h>
int main() {
    volatile long i;
    for (i = 0; i < 84000000000; i++);
    return 0;
}
