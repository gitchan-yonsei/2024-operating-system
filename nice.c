#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char *argv[]) {
    int priority = atoi(argv[2]);
    chpr(priority);
    exit();
}