#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define   MAP_PROT_READ 0x00000001
#define   MAP_PROT_WRITE 0x00000002
#define MAP_FAILED ((void*)-1)

int main(int argc, char *argv[]) {
    int fd;
    char *memory;

    if ((fd = open("README", O_CREATE | O_RDWR)) < 0) {
        printf(1, "Error opening file\n");
        exit();
    }

    // 파일에 데이터를 쓰기 (예: 초기화)
    write(fd, "Hello, xv6!", 12);

    printf(1, "frees (before mmap): %d\n", frees());
    // mmap() 호출
    memory = mmap(fd, 0, 4096, MAP_PROT_READ | MAP_PROT_WRITE);

    printf(1, "frees (after mmap): %d\n", frees());
    if (memory == (char *) -1) {
        printf(1, "mmap failed\n");
        exit();
    }

    printf(1, "Data: %s\n", memory); // 메모리에서 데이터 읽기

    printf(1, "=== unmap test ===");
    munmap(memory, 4096);

    printf(1, "frees (after munmap): %d\n", frees());

    // 파일 닫기
    close(fd);
    exit();
}