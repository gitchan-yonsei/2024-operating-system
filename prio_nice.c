#include "types.h"
#include "stat.h"
#include "user.h"


void prio_test_nice()
{ 
  int pid;
  
  pid = fork();
  
  if (pid == 0) {
    printf(1, "2");
    nice(2);
    printf(1, "4");
    exit();

  } else {
    printf(1, "1");
    nice(1);
    printf(1, "3");
    wait();
  }
}



int main (int argc, char **argv)
{
  printf(1, "====Testing Priority====\n");
  prio_test_nice();
  printf(1, "\n");
  exit();
}
