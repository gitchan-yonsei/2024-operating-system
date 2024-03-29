#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_ITERATIONS 100000000

int
main(void) {
    printf(1, "nice test\n");
    ps();

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
