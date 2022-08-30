#include "types.h"
#include "user.h"
#include "x86.h"


#define NULL 0
#define PGSIZE (4096)

int pthread_create(mpthread_t *thread, mpthread_attr_t *attr, void(*func)(void*), void*arg){
  void *stack = malloc((uint)PGSIZE*2);
  if((uint)stack <= 0){
    printf(1, "Malloc new stack failed in pthread_create\n");
    return -1;
  }
  if((uint)stack % PGSIZE){
    stack += 4096 - ((uint)stack % PGSIZE);
  }
  *thread = clone(func, arg, stack);
  return *thread;
}

int pthread_join(mpthread_t thread, void **retval){
  void *stack;
  if(join((uint)thread, &stack)<0){
    return -1;
  }
  free(stack);
  return 0;
}

void pthread_mutex_init(mpthread_mutex_t *mutex){
  *mutex = 0;
  return;
}

void pthread_mutex_lock(mpthread_mutex_t *mutex){
  while(xchg(mutex, 1) != 0);
  return;
}

void pthread_mutex_unlock(mpthread_mutex_t *mutex){
  xchg(mutex, 0);
  return;
}

void pthread_cond_init(mpthread_cond_t *cond){
  cond->threadId = -1;
  cond->next = NULL;
  return;
}

void pthread_cond_wait(mpthread_cond_t *cond, mpthread_mutex_t *mutex){
  mpthread_cond_t *mycond = cond;
  pthread_mutex_unlock(mutex);
  
  //iterate the linked-list pointer to the end;
  while(mycond->next != NULL)
    mycond = mycond->next;  

  mycond->next = (mpthread_cond_t *)malloc(sizeof(mpthread_cond_t));
  mycond->next->threadId = getpid();
  mycond->next->next = NULL;
  pthread_mutex_lock(mutex);
  pthread_mutex_unlock(mutex);
  threadSleep(mutex);
  pthread_mutex_lock(mutex);
  return;
}

void pthread_cond_signal(mpthread_cond_t *cond){
  mpthread_cond_t *mycond = cond;
  while(mycond->threadId==-1)
    mycond = mycond->next;
  if(mycond!=NULL){
    int threadId = mycond->threadId;
    mpthread_cond_t *garbage = mycond;
    cond->next = cond->next->next;
    free(garbage);
    threadWake(threadId);
  }
  return;
}

void pthread_sem_init(mpthread_sem_t* sem, int flag, int value){
  sem->value = value;
  pthread_cond_init(&sem->cond);
  pthread_mutex_init(&sem->mutex);
  return;
}

void pthread_sem_wait(mpthread_sem_t* sem){
  sem->value --;
  if(sem->value < 0){
    pthread_cond_wait(&sem->cond, &sem->mutex); 
  }
  return;
}

void pthread_sem_post(mpthread_sem_t* sem){
  sem->value ++;
  pthread_cond_signal(&sem->cond);
  return;
}


