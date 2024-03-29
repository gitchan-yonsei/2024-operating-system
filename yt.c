#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void) {
    int i, pid;
    printf(1, "yield test\n");

    pid = fork();
    if (pid == 0) {
        for (i = 0; i < 10; ++i) {
            printf(1, "child\n");
            yield();
            ps();
        }
        exit();
    } else {
        wait();
        for (i = 0; i < 10; ++i) {
            printf(1, " parent\n");
            yield();
            ps();
        }
    }
    printf(1, "finished!\n");
    exit();
}