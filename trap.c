#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "vm.h"
#include "file.h"


// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
//  case T_IRQ0 + IRQ_IDE2:
//	ide2intr();
//	lpaiceoi();
//	break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT:
  {
      uint va = rcr2(); // faulted virtual address
      struct proc *p = myproc();

      if (va >= KERNBASE) {
          cprintf("kernel space 침범");
          myproc()->killed = 1;
          exit();
      }

      if (va >= (myproc()->stack_lower_bound - PGSIZE) && va < myproc()->stack_lower_bound) {
          cprintf("스택가드 영역 침범");
          myproc()->killed = 1;
          exit();
      }

      // mmap 영역 확인
      struct mmap_region *region = 0;
      for (int i = 0; i < p->mmap_count; i++) {
          if ((uint)p->mmap_regions[i].addr <= va &&
              va < (uint)p->mmap_regions[i].addr + p->mmap_regions[i].length) {
              region = &p->mmap_regions[i];
              break;
          }
      }

      if (region) {
          char *mem = kalloc();
          if (!mem) {
              cprintf("메모리 할당 불가");
              p->killed = 1;
              break;
          }
          memset(mem, 0, PGSIZE);

          ilock(region->file->ip);
          readi(region->file->ip, mem, region->offset + (va - (uint)region->addr), PGSIZE);
          iunlock(region->file->ip);

          if (mappages(p->pgdir, (char *)PGROUNDDOWN(va), PGSIZE, V2P(mem), PTE_W | PTE_U) < 0) {
              kfree(mem);
              p->killed = 1;
              break;
          }

          // TLB를 플러시합니다.
          lcr3(V2P(p->pgdir));
          return;
      }

      char *mem = kalloc();
      if (!mem) {
          cprintf("메모리 할당 불가");
          myproc()->killed = 1;
          exit();
      }

      memset(mem, 0, PGSIZE);

      if (mappages(myproc()->pgdir, (char *) PGROUNDDOWN(va), PGSIZE, V2P(mem), PTE_W | PTE_U) < 0) {
          kfree(mem);
          myproc()->killed = 1;
          exit();
      }
  }
          break;
  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
