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