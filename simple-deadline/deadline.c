// https://www.i-programmer.info/programming/cc/13002-applying-c-deadline-scheduling.html?start=1
//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <pthread.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>

// #define SCHED_POLICY SCHED_DEADLINE
#define SCHED_POLICY SCHED_FIFO

struct sched_attr 
{
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};

int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) 
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

void * threadA(void *p) 
{
    struct timespec ts, tm;
    struct timespec startTime;

    if (clock_gettime(CLOCK_MONOTONIC, &startTime) != 0) perror("clock_gettime");

    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        printf("[%ld.%09ld] - Thread A running on CPU %d\n", (long)ts.tv_sec, ts.tv_nsec, sched_getcpu());
    }
    else perror("clock_gettime");

    struct sched_attr attr = 
    {
        .size = sizeof (attr),
        .sched_policy = SCHED_POLICY,
        .sched_runtime = 10 * 1000 * 1000,
        .sched_period = 2 * 1000 * 1000 * 1000,
        .sched_deadline = 11 * 1000 * 1000
    };

    sched_setattr(0, &attr, 0);

    long long elapsed_ns;

    for (;;) 
    {
        if ((clock_gettime(CLOCK_REALTIME, &ts) == 0) && (clock_gettime(CLOCK_MONOTONIC, &tm) == 0)) {
            elapsed_ns = (long long)(tm.tv_sec - startTime.tv_sec) * 1000000000LL + (tm.tv_nsec - startTime.tv_nsec);
            printf("[%ld.%09ld] - Thread A running for %lld ns\n", (long)ts.tv_sec, ts.tv_nsec, elapsed_ns);
        } else {
            perror("clock_gettime");
        }

        fflush(0);

        if(elapsed_ns >= (long long)(6LL * 1000* 1000 * 1000)) // Run for 6s
        {
            fflush(0); 
            break;
        }

        // sched_yield();
        if(sched_yield() != 0) {
            perror("sched_yield"); 
            fflush(0); 
            break;
        }
    };
}


int main(int argc, char** argv) 
{
    pthread_t pthreadA;
    pthread_create(&pthreadA, NULL, threadA, NULL);
    pthread_exit(0);
    return (EXIT_SUCCESS);
}
