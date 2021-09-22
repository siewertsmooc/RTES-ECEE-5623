/*
Function: RT Queueing signal demonstration
Sam Siewert 7/8/97
*/

#include "stdio.h"
#include "stdlib.h"
#include "errnoLib.h"
#include "signal.h"

#define NUMSIGS 10
#define TSIGRTMIN  SIGRTMIN+1
static int sigcount = 0;

static int idle_tid;

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


LOCAL void idle (int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10) {

  struct sigaction sa;

  sa.sa_sigaction = got_user_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  sigcount = 0;

  if(sigaction(TSIGRTMIN, &sa, NULL)) {
    perror("sigaction");
    exit(1);
  }  

  while(1) {
    taskDelay(100);
  }

  exit(0);

}


void rt_signal_test (void) {

  int i;
  union sigval val;

  /* Spawn an idle task to asynchronously signal */
  if ((idle_tid = taskSpawn ("idle", 75, 0, (1024*8), (FUNCPTR) idle, 0,0,0,0,0,0,0,0,0,0)) == ERROR) {
    printf ("Error creating idle task to signal\n");
    return;
  }
  printf ("Idle task to signal has been spawned\n");
  fflush(stdout);


  for(i=0;i<NUMSIGS;i++) {

    val.sival_int = i;
    if((sigqueue(idle_tid, TSIGRTMIN, val)) == ERROR)
      printf("Signal queue error\n");
    else
      printf ("Signal %d thrown with val=%d\n", TSIGRTMIN, i);

    fflush(stdout);

  }

  printf ("\nAll done\n");

  exit(0);
}

