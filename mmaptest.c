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

    if ((fd = open("tempfile", O_CREATE | O_RDWR)) < 0) {
        printf(1, "Error opening file\n");
        exit();
    }

    int size = 4096;
    char data = 0;
    lseek(fd, size - 1, SEEK_SET);
    write(fd, &data, 1);

    memory = mmap(fd, 0, size, MAP_PROT_READ | MAP_PROT_WRITE);
    if (memory == MAP_FAILED) {
        printf(1, "mmap failed\n");
        close(fd);
        exit();
    }

    const char *msg = "Hello, mmap!";
    strcpy(memory, msg);

    // 메모리에서 데이터 읽기 및 확인
    printf(1, "Data from memory: %s\n", memory);

    // 파일 닫기 전에 변경사항 동기화 (선택적)
    msync(memory, size);  // msync가 xv6에 없을 경우 이 라인은 생략

    // 파일 닫기
    close(fd);

    // 파일 다시 열어서 변경 확인
    if ((fd = open("tempfile", O_RDONLY)) < 0) {
        printf(1, "Error reopening file\n");
        exit();
    }

    char buffer[100];
    int n = read(fd, buffer, sizeof(buffer));
    buffer[n] = 0; // Null-terminate the string

    printf(1, "Data read from file: %s\n", buffer);

    close(fd);
    exit();
}