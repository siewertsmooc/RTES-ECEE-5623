#include <pthread.h> // POSIX threads
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <semaphore.h> // Semaphores for synchronization
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
pthread_t threads[2];
threadParams_t threadParams[2];

// Semaphore protecting gsum data
sem_t sem_gsum;

// In general I have noticed higher liklihood of corruption due to read-modify-write race
// conditions on some machines (e.g., NUC cluster nodes) compared to others (e.g., ECC-Linux and
// much more so if you add a larger number than incrementing or decrementing by just 1.
//
// Likewise lots of standard output can slow this down and impact results.
//
// Try enabling and disabling tracing I/O to see the impact.

// Unsafe global
// subject to read-modify-write race condition
int gsum=0;

// Safer ATOMIC global for C-11 compilers
//
// https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/stdatomic.h.html
//
// protects from race conditions on read-modify-write operations
//_Atomic int agsum=0;
atomic_int agsum=0;

// These threads increment global sums COUNT times by INCDEC_AMOUNT.
void *incThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=0; i<COUNT; i++)
    {
        // Now safe using semaphore to protect critical section
        sem_wait(&sem_gsum);

        // Not safe - read-modify-write
        // context switch can occur between these steps and corrupt the result
        gsum=gsum+INCDEC_AMOUNT;

        // Release semaphore to allow dec thread to access gsum
        sem_post(&sem_gsum);

        // Safe - atomic operation prevents race conditions
        // note: return val (not used here) is initial value of agsum
        atomic_fetch_add(&agsum, INCDEC_AMOUNT);
        //agsum=agsum+INCDEC_AMOUNT;

        //printf("Increment thread idx=%d, gsum=%d,  agsum=%d\n", threadParams->threadIdx, gsum, agsum);
    }
    return (void *)0;
}

// These threads decrement global sums COUNT times by INCDEC_AMOUNT.
void *decThread(void *threadp)
{
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=0; i<COUNT; i++)
    {
        // Now safe using semaphore to protect critical section
        sem_wait(&sem_gsum);

        // Not safe - read-modify-write
        // context switch can occur between these steps and corrupt the result
        gsum=gsum-INCDEC_AMOUNT;

        // Release semaphore to allow inc thread to access gsum
        sem_post(&sem_gsum);

        // Safe - atomic operation prevents race conditions
        // note: return val (not used here) is initial value of agsum
        atomic_fetch_sub(&agsum, INCDEC_AMOUNT);
        //agsum=agsum-INCDEC_AMOUNT;

        //printf("Decrement thread idx=%d, gsum=%d, agsum=%d\n", threadParams->threadIdx, gsum, agsum);
    }
    return (void *)0;
}

int main (int argc, char *argv[])
{
   int i=0;

   // init semaphore - shared and initially available
   sem_init(&sem_gsum, 0, 1);

   //create inc thread
   threadParams[i].threadIdx=i;
   pthread_create(&threads[i],   // pointer to thread descriptor
                  (void *)0,     // use default attributes
                  incThread, // thread function entry point
                  (void *)&(threadParams[i]) // parameters to pass in
                 );
   i++;

   //create dec thread
   threadParams[i].threadIdx=i;
   pthread_create(&threads[i], (void *)0, decThread, (void *)&(threadParams[i]));

   // wait for threads to complete
   for(i=0; i<2; i++)
     pthread_join(threads[i], NULL);

   // clean up semaphore
   sem_destroy(&sem_gsum);

   printf("Main thread gsum=%d,  agsum=%d\n", gsum, agsum);
   printf("TEST COMPLETE\n");
}
