#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char *argv[]) {
    int value = atoi(argv[2]);
    nice(value);
    exit();
}