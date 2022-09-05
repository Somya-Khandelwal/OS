<div align="center">

# ASSIGNMENT 1

### Group: M24

### Team members:

<p>

#### Somya Khandelwal - 200123056

#### Pranav Agarwal - 200123040

#### Yashvi Chirag Panchal - 200123073

</p>
</div>

## Part 1: Kernel threads

**thread_create**

This creates a new process as a copy of the current process, but we can distinguish it as a thread by the parent property of "proc" which will store the parent of the thread and also the threads will share the same Page Directory with their parents.
The new process is passed function pointer and stack as parameters and will use 0xffffffff as a fake return address for termination

```
int thread_create(void (_fcn)(void _), void *arg, void *stack)
{
int i, pid;
struct proc *np;
struct proc *curproc = myproc();

    // Allocate process.
    if ((np = allocproc()) == 0)
    {
        return -1;
    }

    np->sz = curproc->sz;
    np->parent = curproc;

    np->pgdir = curproc->pgdir;
    *np->tf = *curproc->tf;
    // *np->context = *curproc->context;   // Saving in context is giving error
    np->tf->eax = 0;
    np->tf->eip = (uint)fcn;
    // np->stack = (uint)stack;
    np->tf->esp = (uint)stack + 4092;
    *((uint *)(np->tf->esp)) = (uint)arg;
    *((uint *)(np->tf->esp - 4)) = 0xFFFFFFFF;
    np->tf->esp -= 4;

    for (i = 0; i < NOFILE; i++)
        if (curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));

    pid = np->pid;

    acquire(&ptable.lock);

    np->state = RUNNABLE;
    release(&ptable.lock);
    return pid;

}
```

**thread_join**

Thread Join blocks the execution of parent thread until the child thread has completed it's execution

```
int thread_join(void)
{

    struct proc *p;
    int havekids, pid;
    struct proc *curproc = myproc();

    acquire(&ptable.lock);

    while (1)
    {
        havekids = 0;
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        {
            if (p->parent != curproc)
                continue;
            havekids = 1;
            if (p->state == ZOMBIE)
            {
                pid = p->pid;
                kfree(p->kstack);
                p->kstack = 0;
                p->state = UNUSED;
                p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                release(&ptable.lock);
                return pid;
            }
        }

        if (!havekids || curproc->killed)
        {
            release(&ptable.lock);
            return -1;
        }
        sleep(curproc, &ptable.lock);
    }

}
```

**thread_exit**

Thread Exit clears the allocated memory, resources, and closes open files on the thread and then finally kills the thread when called.

```
int thread_exit(void)
{

    struct proc *p;
    struct proc *curproc = myproc();
    int fd;

    if (curproc == initproc)
        panic("init exiting");

    for (fd = 0; fd < NOFILE; fd++)
    {
        if (curproc->ofile[fd])
        {
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
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        if (p->parent == curproc)
        {
            p->parent = initproc;
            if (p->state == ZOMBIE)
                wakeup1(initproc);
        }
    }

    // Jump into the scheduler, never to return.
    curproc->state = ZOMBIE;
    sched();
    panic("zombie exit");

}
```

**thread_test**

This is the test function to check the execution of above written code

```
#include "types.h"
#include "stat.h"
#include "user.h"

struct balance
{
    char name[32];
    int amount;
};

volatile int total_balance = 0;
volatile unsigned int delay(unsigned int d)
{
    unsigned int i;
    for (i = 0; i < d; i++)
    {
        __asm volatile("nop" ::
                           :);
    }
    return i;
}

void do_work(void *arg)
{
    int i;
    int old;
    struct balance *b = (struct balance *)arg;
    printf(1, "Starting do_work: s:%s\n", b->name);
    for (i = 0; i < b->amount; i++)
    {
        // thread_spin_lock(&lock);
        old = total_balance;
        delay(100000);
        total_balance = old + 1;
        // thread_spin_unlock(&lock);
    }
    printf(1, "Done s:%s\n", b->name);
    thread_exit();
    return;
}

int main(int argc, char *argv[])
{
    struct balance b1 = {"b1", 3200};
    struct balance b2 = {"b2", 2800};
    void *s1, *s2;
    int t1, t2, r1, r2;
    s1 = malloc(4096);
    s2 = malloc(4096);
    t1 = thread_create(do_work, (void *)&b1, s1);
    t2 = thread_create(do_work, (void *)&b2, s2);
    r1 = thread_join();
    r2 = thread_join();
    printf(1, "Threads finished: (%d):%d, (%d):%d, shared balance:%d\n",
           t1, r1, t2, r2, total_balance);
    exit();
}
```

**output**

![test output](https://i.ibb.co/N1sk2Cj/Whats-App-Image-2022-09-05-at-9-16-43-PM.jpg)
Here we get an output value of 3275 , which clearly is not same as the correct answer, i.e., 6000. This happens due to simultaneous access and changing of the shared variable by both threads. This leads to wrong values being read and updated by the threads.

## Part 2: Synchronization

To solve the problem of race, we have to synchronize both the threads so that only one of them is able to access the shared variable at a time and hence, we do not get inconsistent results

### Spinlock

The spinlock data structure in XV6 has been used to implement spin_locks for the synchronization purpose. It checks for the mutex lock to get free in an infinite loop (hence the name spin), for all the threads accessing the same shared resource ("total_balance" in this case).

### proc.c system_calls

**thread_spinlock_init**
The initialization system call for the spinlock

```
void thread_spinlock_init(struct spinlock *lk)
{
    initlock(lk, "tlock");
}
```

**thread_spin_lock**
This function tries to acquire and lock the shared variable for one thread and hence stops the other thread from access at the same time by sending it in an infite spin.

```
void thread_spin_lock(struct spinlock *lk)
{

    // while(holding(lk));
    // lk->locked = 1;

    /* Using the above commented code was resulting in problems
     due to lack of atomicity
     This resulted in very long busy waits
    */

    // acquire(lk);

    /* Inbuilt acquire and release functions also do not work  */

    while (xchg(&lk->locked, 1))
        ;

    // xchg is atomic and executes lock operation completely
    // before context swit

}
```

**thread_spin_unlock**
Unlock function opens the shared variable for access to other threads once the executing thread has completed its execution

```

void thread_spin_unlock(struct spinlock *lk)
{
    // lk->locked = 0;

    // release(lk);

    asm volatile("movl $0, %0"
                 : "+m"(lk->locked)
                 :);

}
```

**thread_test_spinlock**
This is the test code for synchronised balance calculation using spinlocks

```
#include "types.h"
#include "stat.h"
#include "user.h"
#include "spinlock.h"

struct balance
{
    char name[32];
    int amount;
};

struct spinlock lock;
struct spinlock lock2;

volatile int total_balance = 0;
volatile unsigned int delay(unsigned int d)
{
    unsigned int i;
    for (i = 0; i < d; i++)
    {
        __asm volatile("nop" ::
                           :);
    }
    return i;
}

void do_work(void *arg)
{
    int i;
    int old;
    struct balance *b = (struct balance *)arg;

    thread_spin_lock(&lock2);
    printf(1, "Starting do_work: %s\n", b->name);
    thread_spin_unlock(&lock2);

    for (i = 0; i < b->amount; i++)
    {
        thread_spin_lock(&lock);
        old = total_balance;
        delay(100000);
        total_balance = old + 1;
        thread_spin_unlock(&lock);
    }

    thread_spin_lock(&lock2);
    printf(1, "Done: %s\n", b->name);
    thread_spin_unlock(&lock2);

    thread_exit();
    return;
}

int main(int argc, char *argv[])
{

    thread_spinlock_init(&lock);
    thread_spinlock_init(&lock2);
    struct balance b1 = {"b1", 3200};
    struct balance b2 = {"b2", 2800};
    void *s1, *s2;
    int t1, t2, r1, r2;
    s1 = malloc(4096);
    s2 = malloc(4096);
    t1 = thread_create(do_work, (void *)&b1, s1);
    t2 = thread_create(do_work, (void *)&b2, s2);
    r1 = thread_join();
    r2 = thread_join();
    printf(1, "Threads finished: (%d):%d, (%d):%d, shared balance:%d\n",
           t1, r1, t2, r2, total_balance);
    exit();
}
```

**output**

![spinlock test](https://i.ibb.co/n7PWhjh/Whats-App-Image-2022-09-05-at-9-16-43-PM-1.jpg)
Here we see the final output is 6000, which is consistent with our observations

### Mutex

Mutex lock sends the thread which is unable to acquire the shared variable or resouce to sleep for a fixed amount of time instead of spinning it in an infinite loop. This helps to lower the number of context switches and also fastens the average execution time.

#### mutex.h

This is a simple mutex data structure

```
#include "types.h"

struct mutexlock{
    uint locked;
    char* name;
};
```

#### mutex.c

We have implemented three functions
i) thread_mutex_init : initializes the mutex lock to unlocked state
ii) thread_mutex_lock : locks the mutex for the thread that first requests it
iii) thread_mutex_unlock : unlocks the mutex when a thread is done with critical section exectuion

```
#include "types.h"
#include "user.h"
#include "x86.h"

void thread_mutex_init(struct mutexlock *lk, char *name)
{
    lk->locked = 0;
    lk->name = name;
}

void thread_mutex_lock(struct mutexlock *lk)
{
    while (xchg(&lk->locked, 1))
    {
        sleep(1);
    }
}

void thread_mutex_unlock(struct mutexlock *lk)
{
    asm volatile("movl $0, %0"
                 : "+m"(lk->locked)
                 :);
}
```

#### thread_test_mutex

The test function implemented using mutex locks

```
#include "types.h"
#include "stat.h"
#include "user.h"
#include "mutex.h"
#include "mutex.c"

struct balance
{
    char name[32];
    int amount;
};

struct mutexlock lock;
struct mutexlock lock2;

volatile int total_balance = 0;
volatile unsigned int delay(unsigned int d)
{
    unsigned int i;
    for (i = 0; i < d; i++)
    {
        __asm volatile("nop" ::
                           :);
    }
    return i;
}

void do_work(void *arg)
{
    int i;
    int old;
    struct balance *b = (struct balance *)arg;

    thread_mutex_lock(&lock2);
    printf(1, "Starting do_work: s:%s\n", b->name);
    thread_mutex_unlock(&lock2);

    for (i = 0; i < b->amount; i++)
    {
        thread_mutex_lock(&lock);
        old = total_balance;
        delay(100000);
        total_balance = old + 1;
        thread_mutex_unlock(&lock);
    }

    thread_mutex_lock(&lock2);
    printf(1, "Done s:%s\n", b->name);
    thread_mutex_unlock(&lock2);

    thread_exit();
    return;
}

int main(int argc, char *argv[])
{
    struct balance b1 = {"b1", 3200};
    struct balance b2 = {"b2", 2800};
    void *s1, *s2;
    int t1, t2, r1, r2;
    s1 = malloc(4096);
    s2 = malloc(4096);
    t1 = thread_create(do_work, (void *)&b1, s1);
    t2 = thread_create(do_work, (void *)&b2, s2);
    r1 = thread_join();
    r2 = thread_join();
    printf(1, "Threads finished: (%d):%d, (%d):%d, shared balance:%d\n",
           t1, r1, t2, r2, total_balance);
    exit();
}
```

**output**

![mutex test](https://i.ibb.co/LhStVHr/Whats-App-Image-2022-09-05-at-9-16-43-PM-2.jpg)
Here too, we see that by using process synchronization we get result as 6000, which is consistent with our observations.

### Changes made to xv6 to achieve the above functionality

-   Makefile : The makefile was edited to generate the new function call files at the time of OS loading.
-   defs.h : This is the common header file. Function calls to thread tests, thread_create, thread_join and thread_exit were declared here.
-   sysproc.c : The system calls sys_thread_test, sys_thread_create, sys_thread_join and sys_thread_exit were declared in this file.
-   proc.c : This was the main file for implementing all functions including system calls, spinlock and mutex functionalities.
-   usys.S : The system calls were initialized here in assembly
-   mutex.h : Here, we created the data structure for mutex locks
-   mutex.c : Here, we added all the required functionality to mutex locks
-   user.h : This file stores the declarations for function calls to be used in system calls. We declated thread_create(), thread_join(), thread_exit(), thread_test(), thread_test_spinlock(), thread_test_mutex(), thread_spinlock_init(), thread_spin_lock(), thread_spin_unlock() functions in this file.
