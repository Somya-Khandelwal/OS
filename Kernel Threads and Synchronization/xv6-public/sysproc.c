#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int sys_fork(void)
{
    return fork();
}

int sys_exit(void)
{
    exit();
    return 0; // not reached
}

int sys_wait(void)
{
    return wait();
}

int sys_kill(void)
{
    int pid;

    if (argint(0, &pid) < 0)
        return -1;
    return kill(pid);
}

int sys_getpid(void)
{
    return myproc()->pid;
}

int sys_sbrk(void)
{
    int addr;
    int n;

    if (argint(0, &n) < 0)
        return -1;
    addr = myproc()->sz;
    if (growproc(n) < 0)
        return -1;
    return addr;
}

int sys_sleep(void)
{
    int n;
    uint ticks0;

    if (argint(0, &n) < 0)
        return -1;
    acquire(&tickslock);
    ticks0 = ticks;
    while (ticks - ticks0 < n)
    {
        if (myproc()->killed)
        {
            release(&tickslock);
            return -1;
        }
        sleep(&ticks, &tickslock);
    }
    release(&tickslock);
    return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void)
{
    uint xticks;

    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);
    return xticks;
}

int sys_thread_create(void)
{
    void *stack;
    void *fcn;
    void *bal;

    argptr(0, (void *)&fcn, sizeof(fcn));
    argptr(1, (void *)&bal, sizeof(bal));
    argptr(2, (void *)&stack, sizeof(stack));

    return thread_create(fcn, bal, stack);
}

int sys_thread_join(void)
{
    return thread_join();
}

int sys_thread_exit(void)
{
    return thread_exit();
}

int sys_thread_test(void)
{
    return 0;
}

int sys_thread_test_spinlock(void)
{
    return 0;
}

void sys_thread_spinlock_init(void)
{
    struct spinlock *lk;
    argptr(0, (void *)&lk, sizeof(lk));

    thread_spinlock_init(lk);
}

void sys_thread_spin_lock(void)
{
    struct spinlock *lk;
    argptr(0, (void *)&lk, sizeof(lk));

    thread_spin_lock(lk);
}

void sys_thread_spin_unlock(void)
{
    struct spinlock *lk;
    argptr(0, (void *)&lk, sizeof(lk));

    thread_spin_unlock(lk);
}

int sys_thread_test_mutex(void)
{
    return 0;
}
