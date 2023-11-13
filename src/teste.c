#include <stdio.h>
#include <time.h>
int main() {

    volatile long i = 120000000000;
    while(i--);
    return 0;
}
