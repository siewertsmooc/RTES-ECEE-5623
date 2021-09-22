/*
Function: RT Queueing signal, shared buffer, 
multitasked demonstration

POSIX features used: queueing signals and semaphores

Sam Siewert - 7/8/97
Compiled and tested for: VxWorks 5.3
*/

#include "stdio.h"
#include "stdlib.h"
#include "signal.h"
#include "semaphore.h"
#include "errnoLib.h"
#include "sys/fcntlcom.h"

static int idle_tid;
static shared_buffer[256];

#define NUMSIGS 10
#define TSIGRTMIN  SIGRTMIN+1
static int sigcount = 0;

static sem_t *sbsem;
static int exit_thread = 0;

void got_user_signal(int signo, siginfo_t *info, void *ignored)
{
  void *ptr_val = info->si_value.sival_ptr;
  int int_val = info->si_value.sival_int;

  sigcount++;

  printf("Caught user signal %d %d times with val=%d\n", 
         signo, sigcount, int_val);

  taskDelay(100);

  if(sigcount == NUMSIGS)
    taskDelete(idle_tid);

  return;
}


void idle (int arg1, int arg2, int arg3, int arg4, int arg5,int arg6, int arg7, int arg8, int arg9, int arg10) {

  struct sigaction sa;

  sa.sa_sigaction = got_user_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  sigcount = 0;

  if(sigaction(TSIGRTMIN, &sa, NULL)) {
    perror("sigaction");
    exit(1);
  }  

  while(!exit_thread) {

    /* this thread will block all user level threads */
    taskDelay(100);

    /* CRITICAL SECTION -- read and write of shared buffer */
    if(sem_wait(sbsem) == ERROR)
      perror("sem_wait");
    else {
      printf("*****CS Shared buffer = %s\n", 
shared_buffer);
      strcpy(shared_buffer, "idle task was 
here!");
    }
    if(sem_post(sbsem) == ERROR)
      perror("sem_post");


    fflush(stdout);

  }

  exit(0);

}


void rt_posix_test (void) {

  int i;
  union sigval val;

/* Setup POSIX binary semaphore */

sbsem = (sem_t *)malloc(sizeof(sem_t));

if(sem_init(sbsem, 1, 1) == ERROR) {
  perror("semaphore create");
}

  /* Spawn an idle task to asynchronously 
signal */
  if ((idle_tid = taskSpawn ("idle", 75, 0, (1024*8), (FUNCPTR) idle, 0,0,0,0,0,0,0,0,0,0)) == ERROR) {
    printf ("Error creating idle task to signal\n");
    return;
  }

  printf ("Signal generator: Idle task/process to signal has been spawned\n");
  fflush(stdout);


  for(i=0;i<NUMSIGS;i++) {

    val.sival_int = i;
    if((sigqueue(idle_tid, TSIGRTMIN, val)) == ERROR)
      printf("Signal queue error\n");
    else
      printf ("Signal %d thrown with val=%d\n", TSIGRTMIN, i);

    /* CRITICAL SECTION -- read and write of shared buffer */
    if(sem_wait(sbsem) == ERROR)
      perror("sem_wait");
    else {
      printf("*****CS Shared buffer = %s\n", shared_buffer);
      strcpy(shared_buffer, "signal task was here!");
    }
    if(sem_post(sbsem) == ERROR)
      perror("sem_post");

    fflush(stdout);

    if(!(i%3))  {
      taskDelay(100);
    }

  }


  if(sem_destroy(sbsem) == ERROR)
    perror("sem destroy");

  printf ("\nAll done\n");

  exit(0);
}
