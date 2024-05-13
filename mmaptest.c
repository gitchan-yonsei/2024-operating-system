#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define MAP_PROT_READ 0x00000001
#define MAP_PROT_WRITE 0x00000002
#define MAP_FAILED ((void*)-1)

int main(int argc, char *argv[]) {
    int fd;
    char *memory;

    // 파일 열기 (생성 없이 오직 읽기/쓰기)
    if ((fd = open("tempfile", O_RDWR)) < 0) {
        printf(1, "Error opening file\n");
        exit();
    }

    // 파일 크기를 4096 바이트로 설정 (초기화)
    // xv6에서 파일 크기를 확장하는 방법이 필요합니다.
    // 이 예제에서는 mmap을 사용하여 메모리 매핑을 시도하기 전에
    // 파일이 존재하고 적절한 크기를 가지고 있어야 합니다.

    // mmap() 호출
    memory = mmap(fd, 0, 4096, MAP_PROT_READ | MAP_PROT_WRITE);
    if (memory == MAP_FAILED) {
        printf(1, "mmap failed\n");
        close(fd);
        exit();
    }

    // 메모리 매핑된 영역에 데이터 쓰기
    const char *msg = "Hello, mmap!";
    strcpy(memory, msg);

    // 메모리에서 데이터 읽기 및 확인
    printf(1, "Data from memory: %s\n", memory);

    // 파일 닫기
    close(fd);
    exit();
}
