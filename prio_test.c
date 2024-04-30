#include "types.h"
#include "stat.h"
#include "user.h"




void prio_test()
{ 
  // Add your own scenarios to test other scheduling points
  int pid;
  
  pid = fork();
  
  if (pid == 0) {
    printf(1, "[Child: 2]\n");
    nice(2);
    printf(1, "[Child: 4]\n");
    exit();

  } else {
    printf(1, "[Parent: 1]\n");
    nice(1);
    printf(1, "[Parent: 3]\n");
    wait();
  }
}



int main (int argc, char **argv)
{
  printf(1, "====Testing Priority Scheduler====\n");
  prio_test();
  printf(1, "====Finished Testing====\n");
  exit();
}
