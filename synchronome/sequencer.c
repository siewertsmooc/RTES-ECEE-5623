/*  Sam Siewert, September 2016

    Check to ensure all your CPU cores on in an online state.

    Check /sys/devices/system/cpu or do lscpu.

    Ideally all printf calls should be eliminated as they can interfere with
    timing.  They should be replaced with an in-memory event logger or at least
    calls to syslog.

    This code has been adapted from the original version by Lorin Achey.
    The original author and source code can be found at: https://github.com/siewertsmooc/RTES-ECEE-5623

    This is necessary for CPU affinity macros in Linux
*/
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <semaphore.h>

#include <syslog.h>
#include <sys/sysinfo.h>

#define USEC_PER_MSEC (1000)
#define NUM_CPU_CORES (1)
#define FIB_TEST_CYCLES (100)
#define NUM_THREADS (4) // service threads + sequencer
sem_t semF10, semFrameSelectionService, semFrameWriteBackService;

#define FIB_LIMIT_FOR_32_BIT (47)
#define FIB_LIMIT (10)

int abortTest = 0;
double start_time;

unsigned int seqIterations = FIB_LIMIT;
unsigned int idx = 0, jdx = 1;
unsigned int fib = 0, fib0 = 0, fib1 = 1;

double getTimeMsec(void);

#define FIB_TEST(seqCnt, iterCnt)       \
    for (idx = 0; idx < iterCnt; idx++) \
    {                                   \
        fib0 = 0;                       \
        fib1 = 1;                       \
        jdx = 1;                        \
        fib = fib0 + fib1;              \
        while (jdx < seqCnt)            \
        {                               \
            fib0 = fib1;                \
            fib1 = fib;                 \
            fib = fib0 + fib1;          \
            jdx++;                      \
        }                               \
    }

typedef struct
{
    int threadIdx;
    int MajorPeriods;
} threadParams_t;


void *fib10(void *threadp)
{
    double event_time, run_time = 0.0;
    int limit = 0, release = 0, cpucore, i;
    threadParams_t *threadParams = (threadParams_t *)threadp;
    unsigned int required_test_cycles;

    required_test_cycles = (int)(10.0 / run_time);
    printf("F10 runtime calibration %lf msec per %d test cycles, so %u required\n", run_time, FIB_TEST_CYCLES, required_test_cycles);

    while (!abortTest)
    {
        sem_wait(&semF10);

        if (abortTest)
            break;
        else
            release++;

        cpucore = sched_getcpu();
        printf("F10 start %d @ %lf on core %d\n", release, (event_time = getTimeMsec() - start_time), cpucore);

        do
        {
            FIB_TEST(seqIterations, FIB_TEST_CYCLES);
            limit++;
        } while (limit < required_test_cycles);

        printf("F10 complete %d @ %lf, %d loops\n", release, (event_time = getTimeMsec() - start_time), limit);
        limit = 0;
    }

    pthread_exit((void *)0);
}

void *frameSelectionService(void *threadp)
{
    double event_time, run_time = 0.0;
    int limit = 0, release = 0, cpucore, i;
    threadParams_t *threadParams = (threadParams_t *)threadp;
    int required_test_cycles = (int)(20.0 / run_time);

    while (!abortTest)
    {
        sem_wait(&semFrameSelectionService);

        if (abortTest)
            break;
        else
            release++;

        cpucore = sched_getcpu();
        printf("frameSelectionService start %d @ %lf on core %d\n", release, (event_time = getTimeMsec() - start_time), cpucore);
        syslog(LOG_CRIT, "SYSLOG_RT_TRC: frameSelectionService start time in milliseconds: %lf\n", (event_time = getTimeMsec() - start_time));

        do
        {
            FIB_TEST(seqIterations, FIB_TEST_CYCLES);
            limit++;
        } while (limit < required_test_cycles);

        printf("frameSelectionService complete %d @ %lf, %d loops\n", release, (event_time = getTimeMsec() - start_time), limit);
        limit = 0;
    }

    pthread_exit((void *)0);
}

void *frameWriteBackService(void *threadp)
{
    double event_time, run_time = 0.0;
    int limit = 0, release = 0, cpucore, i;
    threadParams_t *threadParams = (threadParams_t *)threadp;
    int required_test_cycles = (int)(20.0 / run_time);

    event_time = getTimeMsec();

    while (!abortTest)
    {
        sem_wait(&semFrameWriteBackService);

        if (abortTest)
            break;
        else
            release++;

        cpucore = sched_getcpu();
        printf("frameWriteBack service start %d @ %lf on core %d\n", release, (event_time = getTimeMsec() - start_time), cpucore);

        do
        {
            FIB_TEST(seqIterations, FIB_TEST_CYCLES);
            limit++;
        } while (limit < required_test_cycles);

        printf("frameWriteBack service complete %d @ %lf, %d loops\n", release, (event_time = getTimeMsec() - start_time), limit);
        limit = 0;
    }

    pthread_exit((void *)0); 
}

double getTimeMsec(void)
{
    struct timespec event_ts = {0, 0};

    clock_gettime(CLOCK_MONOTONIC, &event_ts);
    return ((event_ts.tv_sec) * 1000.0) + ((event_ts.tv_nsec) / 1000000.0);
}

void print_scheduler(void)
{
    int schedType;

    schedType = sched_getscheduler(getpid());

    switch (schedType)
    {
    case SCHED_FIFO:
        printf("Pthread Policy is SCHED_FIFO\n");
        break;
    case SCHED_OTHER:
        printf("Pthread Policy is SCHED_OTHER\n");
        exit(-1);
        break;
    case SCHED_RR:
        printf("Pthread Policy is SCHED_RR\n");
        exit(-1);
        break;
    default:
        printf("Pthread Policy is UNKNOWN\n");
        exit(-1);
    }
}

void *Sequencer(void *threadp)
{
    int i;
    int MajorPeriodCnt = 0;
    double event_time;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    printf("Starting Sequencer\n");
    start_time = getTimeMsec();

    syslog(LOG_CRIT, "SYSLOG_RT_TRC: Sequencer start time in milliseconds: %lf\n", start_time);

    do
    {
        // Basic sequence of releases after CI for 90% load
        //
        // S1 (Frame Acquisition Service): T1= 20 -> Runs every 50 milliseconds
        // S2 (Frame Selection): T2= 2 -> Runs every 500 milliseconds
        // S3 (Frame W/B): T2= 2 -> Runs every 500 milliseconds
        //
        // Use of usleep is not ideal, but is sufficient for predictable response.
        //
        // To improve, use a real-time clock (programmable interval time with an
        // ISR) which in turn raises a signal (software interrupt) to a handler
        // that performs the release.
        //

        // Simulate the C.I. for S1, S2, S3 and timestamp in log
        printf("\n**** CI t=%lf\n", event_time = getTimeMsec() - start_time);
        sem_post(&semF10); //TODO: Remove this comment but for now pretend F10 is Frame Acquisition
        sem_post(&semFrameSelectionService);
        sem_post(&semFrameWriteBackService);

        usleep(20 * USEC_PER_MSEC);
        sem_post(&semF10);
        printf("t=%lf\n", event_time = getTimeMsec() - start_time);

        usleep(500 * USEC_PER_MSEC);
        sem_post(&semFrameSelectionService);
        printf("t=%lf\n", event_time = getTimeMsec() - start_time);

        usleep(500 * USEC_PER_MSEC);
        sem_post(&semFrameWriteBackService);
        printf("t=%lf\n", event_time = getTimeMsec() - start_time);

        MajorPeriodCnt++;
    } while (MajorPeriodCnt < threadParams->MajorPeriods);

    abortTest = 1;
    sem_post(&semF10);
    sem_post(&semFrameSelectionService);
    sem_post(&semFrameWriteBackService);
}

void main(void)
{
    int i, rc, scope;
    cpu_set_t threadcpu;
    pthread_t threads[NUM_THREADS];
    threadParams_t threadParams[NUM_THREADS];
    pthread_attr_t rt_sched_attr[NUM_THREADS];
    int rt_max_prio, rt_min_prio;
    struct sched_param rt_param[NUM_THREADS];
    struct sched_param main_param;
    pthread_attr_t main_attr;
    pid_t mainpid;
    cpu_set_t allcpuset;

    abortTest = 0;

    printf("System has %d processors configured and %d available.\n", get_nprocs_conf(), get_nprocs());

    CPU_ZERO(&allcpuset);

    for (i = 0; i < NUM_CPU_CORES; i++)
        CPU_SET(i, &allcpuset);

    printf("Using CPUS=%d from total available.\n", CPU_COUNT(&allcpuset));

    // initialize the sequencer semaphores
    //
    if (sem_init(&semF10, 0, 0))
    {
        printf("Failed to initialize semF10 semaphore\n");
        exit(-1);
    }
    if (sem_init(&semFrameSelectionService, 0, 0))
    {
        printf("Failed to initialize semFrameSelectionService semaphore\n");
        exit(-1);
    }
    if (sem_init(&semFrameWriteBackService, 0, 0))
    {
        printf("Failed to initialize semFrameWriteBackService semaphore\n");
        exit(-1);
    }

    mainpid = getpid();

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    rc = sched_getparam(mainpid, &main_param);
    main_param.sched_priority = rt_max_prio;
    rc = sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    if (rc < 0)
        perror("main_param");
    print_scheduler();

    pthread_attr_getscope(&main_attr, &scope);

    if (scope == PTHREAD_SCOPE_SYSTEM)
        printf("PTHREAD SCOPE SYSTEM\n");
    else if (scope == PTHREAD_SCOPE_PROCESS)
        printf("PTHREAD SCOPE PROCESS\n");
    else
        printf("PTHREAD SCOPE UNKNOWN\n");

    printf("rt_max_prio=%d\n", rt_max_prio);
    printf("rt_min_prio=%d\n", rt_min_prio);

    for (i = 0; i < NUM_THREADS; i++)
    {

        CPU_ZERO(&threadcpu);
        CPU_SET(3, &threadcpu);

        rc = pthread_attr_init(&rt_sched_attr[i]);
        rc = pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
        rc = pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
        rc = pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

        rt_param[i].sched_priority = rt_max_prio - i;
        pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

        threadParams[i].threadIdx = i;
    }

    printf("Service threads will run on %d CPU cores\n", CPU_COUNT(&threadcpu));

    // Create Service threads which will block awaiting release for:
    //
    // serviceF10 - Frame Acquisition
    rc = pthread_create(&threads[1],                 // pointer to thread descriptor
                        &rt_sched_attr[1],           // use specific attributes
                        //(void *)0,                 // default attributes
                        fib10,                       // thread function entry point
                        (void *)&(threadParams[1])   // parameters to pass in
    );
    // serviceF20 - Frame Select
    rc = pthread_create(&threads[2],                 // pointer to thread descriptor
                        &rt_sched_attr[2],           // use specific attributes
                        //(void *)0,                 // default attributes
                        frameSelectionService,       // thread function entry point
                        (void *)&(threadParams[2])   // parameters to pass in
    );
    // serviceF20 - Frame WriteBack
    rc = pthread_create(&threads[3],                 // pointer to thread descriptor
                        &rt_sched_attr[3],           // use specific attributes
                        //(void *)0,                 // default attributes
                        frameSelectionService,       // thread function entry point
                        (void *)&(threadParams[3])   // parameters to pass in
    );

    // Wait for service threads to calibrate and await relese by sequencer
    usleep(300000);

    // Create Sequencer thread, which like a cyclic executive, is highest prio
    printf("Start sequencer\n");
    threadParams[0].MajorPeriods = 10;

    rc = pthread_create(&threads[0],                 // pointer to thread descriptor
                        &rt_sched_attr[0],           // use specific attributes
                        //(void *)0,                 // default attributes
                        Sequencer,                   // thread function entry point
                        (void *)&(threadParams[0])   // parameters to pass in
    );

    for (i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("\nTEST COMPLETE\n");
}
