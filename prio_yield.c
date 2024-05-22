#include "types.h"
#include "stat.h"
#include "user.h"


void prio_test_yield()
{
  int pid1, pid2;
  pid1 = fork();
  if (pid1 == 0) {
    // child1
    printf(1, "3");
    yield();
    printf(1, "4");
    exit();

  } else {
    pid2 = fork();
    if (pid2 == 0) {
      // child2
      printf(1, "5");
      exit();

    }
    printf(1, "1");
    yield();
    printf(1, "2");
    wait();
    wait();
    printf(1, "6");
  }
}


int main (int argc, char **argv)
{
  printf(1, "====Testing====\n");
  prio_test_yield();
  printf(1, "\n");
  exit();
}
