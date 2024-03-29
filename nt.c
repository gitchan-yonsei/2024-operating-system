#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_ITERATIONS 100000000

void work(int nice_val) {
    int i, x = 0;
    for (i = 0; i < NUM_ITERATIONS; i++) {
        x = x + 1;
    }
    printf(1, "Nice value: %d, Result: %d\n", nice_val, x);
}

int
main(void) {
    printf(1, "nice test\n");

    int pid = fork();
    if (pid == 0) {
        nice(2);
        ps();
        exit();
    } else {
        nice(-2);
        ps();
        wait();
    }

    exit();
}
