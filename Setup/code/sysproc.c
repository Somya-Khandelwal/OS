#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
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
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


int sys_draw(){
	char* buf;
	uint size;
	if(argptr(0, (void*)&buf, sizeof(buf)) < 0) return -1;
	if(argptr(0, (void*)&size, sizeof(size)) < 0) return -1;
	
	static char turt[] = \
"                         \n"
"       /^\\  \n"
"      |   |			\n"
"/\\     |_|     /\\		\n"
"| \\___/' `\\___/ |	\n"
" \\_/  \\___/  \\_/	\n"
"  |\\__/   \\__/|	\n"
"  |/  \\___/  \\|	\n"
" ./\\__/   \\__/\\,	\n"
" | /  \\___/  \\ |	\n"
" \\/     V     \\/   \n"
"                          \n";
    					  
    static uint turt_len = sizeof(turt);
    
    if(size < turt_len) return -1;
    
    int i = 0;
    while(turt[i] != '\0'){
     	buf[i] = turt[i];
    	++i;
    }
    buf[i] = '\0';
    
    return turt_len;
}
