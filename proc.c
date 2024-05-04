#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define HIGH 0
#define MEDIUM 1
#define LOW 2
#define NUM_QUEUES 3
#define MAX_TICKS 4

struct proc* q0[NPROC];
struct proc* q1[NPROC];
struct proc* q2[NPROC];

int c0 = -1;
int c1 = -1;
int c2 = -1;

int clkPerPrio[4] = {4, 4, 4, 4};

//void enqueue(struct proc *p) {
//    int priority = p->priority;
//    queue[priority][queue_count[priority]++] = p;
//}
//
//struct proc* dequeue(int priority) {
//    if (queue_count[priority] == 0) return 0;
//    struct proc* p = queue[priority][0];
//    for(int i = 0; i < queue_count[priority] - 1; i++) {
//        queue[priority][i] = queue[priority][i + 1];
//    }
//    queue_count[priority]--;
//    return p;
//}

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == UNUSED) {
            goto found;
        }
    }

    p->priority = 0;
    p->ticks = 0;
    c0++;
    q0[c0] = p;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->priority = 0;
  p->ticks = 0;
  c0++;
  q0[c0] = p;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  cprintf("%p %p\n", _binary_initcode_start, _binary_initcode_size);
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S
  p->nice = 2;

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  np->nice = curproc->nice;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  np->priority = 0;
  np->ticks = 0;
  c0++;
  q0[c0] = np;

    if (myproc()->state == RUNNING) {
        myproc()->state = RUNNABLE;
    }

    sched();

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void scheduler(void)
{

    struct proc *p;
    int i;
    int j;
    for(;;){
        // Enable interrupts on this processor.
        sti();

        // Loop over process table looking for process to run.
        acquire(&ptable.lock);


        if(c0!=-1){

            for (i = 0; i <= c0; i++) {
                if (q0[i]->state != RUNNABLE)
                    continue;
                p = q0[i];
                mycpu()->proc = q0[i];
                p->ticks++;
                switchuvm(p);
                p->state = RUNNING;
                swtch(&(mycpu()->scheduler), mycpu()->proc->context);
                switchkvm();
                if (p->ticks == clkPerPrio[0]) {
                    /*copy proc to lower priority queue*/
                    c1++;
                    mycpu()->proc->priority = mycpu()->proc->priority + 1;
                    q1[c1] = mycpu()->proc;

                    /*delete proc from q0*/
                    q0[i] = 0;
                    for (j = i; j <= c0 - 1; j++)
                        q0[j] = q0[j + 1];
                    q0[c0] = 0;
                    mycpu()->proc->ticks = 0;
                    c0--;
                }

                mycpu()->proc = 0;
            }
        }
        if(c1!=-1){
            for(i=0;i<=c1;i++){
                if(q1[i]->state != RUNNABLE)
                    continue;

                p=q1[i];
                mycpu()->proc = q1[i];
                mycpu()->proc->ticks++;
                switchuvm(p);
                p->state = RUNNING;
                swtch(&(mycpu()->scheduler), mycpu()->proc->context);
                switchkvm();
                if(p->ticks ==clkPerPrio[1]){

                    /*copy proc to lower priority queue*/
                    c2++;
                    mycpu()->proc->priority=mycpu()->proc->priority+1;
                    q2[c2] = mycpu()->proc;

                    /*delete proc from q0*/
                    q1[i]=0;
                    for(j=i;j<=c1-1;j++)
                        q1[j] = q1[j+1];
                    q1[c1] = 0;
                    mycpu()->proc->ticks = 0;
                    c1--;
                }
                mycpu()->proc = 0;
            }
        }

        if(c2!=-1){
            for(i=0;i<=c2;i++){
                if(q2[i]->state != RUNNABLE)
                    continue;

                p=q2[i];
                mycpu()->proc = q2[i];
                mycpu()->proc->ticks++;
                switchuvm(p);
                p->state = RUNNING;
                swtch(&(mycpu()->scheduler), mycpu()->proc->context);
                switchkvm();

                /*move process to end of its own queue */
                q2[i]=0;
                for(j=i;j<=c2-1;j++)
                    q2[j] = q2[j+1];
                q2[c2] = mycpu()->proc;

                mycpu()->proc = 0;
            }
        }
        
        release(&ptable.lock);
    }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
    struct proc *p;
    int i;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == SLEEPING && p->chan == chan){
            p->ticks = 0;
            p->state = RUNNABLE;
            if(p->priority == 0) {
                c0++;
                for(i=c0;i>0;i--) {
                    q0[i] = q0[i-1];
                }
                q0[0] = p;
            }
            else if(p->priority == 1) {
                c1++;
                for(i=c1;i>0;i--) {
                    q1[i] = q1[i-1];
                }
                q1[0] = p;
            }
            else if(p->priority == 2) {
                c2++;
                for(i=c2;i>0;i--) {
                    q2[i] = q2[i-1];
                }
                q2[0] = p;
            }
//            else  {
//                c3++;
//                for(i=c3;i>0;i--) {
//                    q3[i] = q3[i-1];
//                }
//                q3[0] = p;
//            }

            for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
                if (p->state == SLEEPING && p->chan == chan)
                    p->state = RUNNABLE;
        }
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);

//  struct proc *curproc = myproc();
//  if (curproc->state == RUNNING) {
//      yield();
//  }
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

int nice(int value)
{
	int new_nice;
	struct proc *p = myproc();
	acquire(&ptable.lock);
	new_nice = p->nice + value;
	if ( new_nice > 4 )
		new_nice = 4;
	else if (new_nice < -5 )
		new_nice = -5;
	p->nice = new_nice;
	release(&ptable.lock);

	return p->nice;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %d %s %s", p->pid, p->nice, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int state(enum procstate state) {
    switch (state) {
        case UNUSED:
            return 0;
        case EMBRYO:
            return 1;
        case SLEEPING:
            return 2;
        case RUNNABLE:
            return 3;
        case RUNNING:
            return 4;
        case ZOMBIE:
            return 5;
        default:
            return -1;
    }
}

int
ps() {
    struct proc *p;

    acquire(&ptable.lock);

    cprintf("name \t pid \t state \t nice \t ticks \t ticks: %d\n", ticks);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        int stateNumber = state(p->state);
        if (stateNumber == 2 || stateNumber == 3 || stateNumber == 4 || stateNumber == 5) {
            cprintf("%s \t %d \t %d \t %d \t %d \n ", p->name, p->pid, stateNumber, p->nice, p->ticks);
        }
    }

    release(&ptable.lock);

    return 0;
}
