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
    printf(1, "Nice test\n");

    int pid = fork();
    if (pid == 0) {
        // 자식 프로세스
        nice(10); // 자식 프로세스의 우선순위를 낮춤
        work(10);
    } else if (pid > 0) {
        // 부모 프로세스
        nice(0); // 부모 프로세스의 우선순위는 그대로 유지
        work(0);
        wait(); // 자식 프로세스의 종료를 기다림
    } else {
        // fork 실패
        printf(1, "fork failed\n");
        exit();
    }

    exit();
}
