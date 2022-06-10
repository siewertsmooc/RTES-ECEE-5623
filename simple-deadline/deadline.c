// https://www.i-programmer.info/programming/cc/13002-applying-c-deadline-scheduling.html?start=1
//

/*
This code has been adapated from Dr. Sam Siewart's example code, which is accessibly
via the following github link: https://github.com/siewertsmooc/RTES-ECEE-5623
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <pthread.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/syscall.h>

#define SCHED_POLICY SCHED_DEADLINE
#define MY_CLOCK CLOCK_MONOTONIC
#define TEST_ITERATIONS (100)

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

static struct timespec rtclk_time = {0, 0};

int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) 
{
    return syscall(__NR_sched_setattr, pid, attr, flags);
}

void * threadA(void *p) 
{
    struct sched_attr attr = 
    {
        .size = sizeof (attr),
        .sched_policy = SCHED_POLICY,
        .sched_runtime = 10 * 1000 * 1000,
        .sched_period = 2 * 1000 * 1000 * 1000,
        .sched_deadline = 11 * 1000 * 1000
    };

    sched_setattr(0, &attr, 0);

    for (int i = 0; i < TEST_ITERATIONS; i++) 
    {
        clock_gettime(MY_CLOCK, &rtclk_time);
        printf("\n\nPOSIX Clock demo using system RT clock with resolution:\n\t%ld secs, %ld microsecs, %ld nanosecs\n", rtclk_time.tv_sec, (rtclk_time.tv_nsec/1000), rtclk_time.tv_nsec);
        fflush(0);
        sched_yield();
    };
}


int main(int argc, char** argv) 
{
    pthread_t pthreadA;
    pthread_create(&pthreadA, NULL, threadA, NULL);
    pthread_exit(0);
    return (EXIT_SUCCESS);
}
