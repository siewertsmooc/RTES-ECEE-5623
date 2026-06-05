#define _GNU_SOURCE // for getcpu
#include <pthread.h> // POSIX threads
#include <stdlib.h>
#include <stdio.h>
#include <sched.h> //fifo synchronization
#include <semaphore.h> // Semaphores for synchronization
#include <unistd.h> // getpid
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

// Attribute for fifo scheduling policy
pthread_attr_t fifo_sched_attr;
struct sched_param fifo_param;

// These threads increment global sums COUNT times by INCDEC_AMOUNT.
void *incThread(void *threadp)
{
    printf("Inc thread running on CPU %d\n", sched_getcpu());
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=0; i<COUNT; i++)
    {
        // Not safe - read-modify-write
        // context switch can occur between these steps and corrupt the result
        gsum=gsum+INCDEC_AMOUNT;

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
    printf("Dec thread running on CPU %d\n", sched_getcpu());
    int i;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    for(i=0; i<COUNT; i++)
    {
        // Not safe - read-modify-write
        // context switch can occur between these steps and corrupt the result
        gsum=gsum-INCDEC_AMOUNT;

        // Safe - atomic operation prevents race conditions
        // note: return val (not used here) is initial value of agsum
        atomic_fetch_sub(&agsum, INCDEC_AMOUNT);
        //agsum=agsum-INCDEC_AMOUNT;

        //printf("Decrement thread idx=%d, gsum=%d, agsum=%d\n", threadParams->threadIdx, gsum, agsum);
    }
    return (void *)0;
}


void set_scheduler(void)
{
    int max_prio, cpuidx;
    cpu_set_t cpuset;

    pthread_attr_init(&fifo_sched_attr);
    pthread_attr_setinheritsched(&fifo_sched_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&fifo_sched_attr, SCHED_FIFO);

    CPU_ZERO(&cpuset);
    cpuidx=(3);
    CPU_SET(cpuidx, &cpuset);
    pthread_attr_setaffinity_np(&fifo_sched_attr, sizeof(cpu_set_t), &cpuset);

    max_prio=sched_get_priority_max(SCHED_FIFO);
    fifo_param.sched_priority=max_prio;    

    if(0 > sched_setscheduler(getpid(), SCHED_FIFO, &fifo_param))
        perror("sched_setscheduler");

    pthread_attr_setschedparam(&fifo_sched_attr, &fifo_param);
}

int main (int argc, char *argv[])
{
    printf("Main thread running on CPU %d\n", sched_getcpu());
    int i=0;

    set_scheduler();

    //create inc thread
    threadParams[i].threadIdx=i;
    pthread_create(&threads[i],   // pointer to thread descriptor
                    &fifo_sched_attr,     // use FIFO max priority attributes
                    incThread, // thread function entry point
                    (void *)&(threadParams[i]) // parameters to pass in
                    );
    i++;

    //create dec thread
    threadParams[i].threadIdx=i;
    pthread_create(&threads[i], &fifo_sched_attr, decThread, (void *)&(threadParams[i]));

    // wait for threads to complete
    for(i=0; i<2; i++)
        pthread_join(threads[i], NULL);

    printf("Main thread gsum=%d,  agsum=%d\n", gsum, agsum);
    printf("TEST COMPLETE\n");
}
