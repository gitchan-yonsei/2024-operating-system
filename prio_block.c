#include "types.h"
#include "stat.h"
#include "user.h"


void prio_test_block()
{
  int pid1, pid2;
  
  pid1 = fork();
  
  if (pid1 == 0) {
    // child1
    printf(1, "2");
    volatile unsigned int sum = 0;
    unsigned int i;
    for (i=0; i<10000000; i++) {
      sum += i;
    }
    printf(1, "4");
    exit();

  } else {
    // parent: fork again
    pid2 = fork();
    if (pid2 == 0) {
      // child2
      printf(1, "5");
      exit();
    }

    printf(1, "1");
    sleep(1);
    printf(1, "3");
    wait();
    wait();
    printf(1, "6");
	
  }
}



int main (int argc, char **argv)
{
  printf(1, "====Testing====\n");
  prio_test_block();
  printf(1, "\n====Testing Done====\n");
  exit();
}
