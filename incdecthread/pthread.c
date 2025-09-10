#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>

#include <stdatomic.h>

// If you test at more than 1 billion and/or with a larger INCDEC_AMOUNT, you
// risk an overflow.
//
#define COUNT  (1000000)
//#define COUNT  (1000000000)

#define INCDEC_AMOUNT (1)

typedef struct
{
    int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
//
pthread_t threads[2];
threadParams_t threadParams[2];


// In general I have noticed higher liklihood of corruption due to read-modify-write race
// conditions on some machines (e.g., NUC cluster nodes) compared to others (e.g., ECC-Linux and
// much more so if you add a larger number than incrementing or decrementing by just 1.
//
// Likewise lots of standard output can slow this down and impact results.
//
// Try enabling and disabling tracing I/O to see the impact.

// Unsafe global
int gsum=0;

// Safer ATOMIC global for C-11 compilers
//
// https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/stdatomic.h.html
//
//_Atomic int agsum=0;
atomic_int agsum=0;

void *incThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=0; i<COUNT; i++)
    {
        gsum=gsum+INCDEC_AMOUNT;

        atomic_fetch_add(&agsum, INCDEC_AMOUNT);
        //agsum=agsum+INCDEC_AMOUNT;

        //printf("Increment thread idx=%d, gsum=%d,  agsum=%d\n", threadParams->threadIdx, gsum, agsum);
    }
    return (void *)0;
}


void *decThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=0; i<COUNT; i++)
    {
        gsum=gsum-INCDEC_AMOUNT;

        atomic_fetch_sub(&agsum, INCDEC_AMOUNT);
        //agsum=agsum-INCDEC_AMOUNT;

        //printf("Decrement thread idx=%d, gsum=%d, agsum=%d\n", threadParams->threadIdx, gsum, agsum);
    }
    return (void *)0;
}




int main (int argc, char *argv[])
{
   int i=0;

   threadParams[i].threadIdx=i;
   pthread_create(&threads[i],   // pointer to thread descriptor
                  (void *)0,     // use default attributes
                  incThread, // thread function entry point
                  (void *)&(threadParams[i]) // parameters to pass in
                 );
   i++;

   threadParams[i].threadIdx=i;
   pthread_create(&threads[i], (void *)0, decThread, (void *)&(threadParams[i]));

   for(i=0; i<2; i++)
     pthread_join(threads[i], NULL);

   printf("Main thread gsum=%d,  agsum=%d\n", gsum, agsum);
   printf("TEST COMPLETE\n");
}
