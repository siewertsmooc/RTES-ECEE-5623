//
// Sequencer Generic to emulate Example 0 assuming millisec time resolution
//
// Service_1, S1, T1=2,  C1=1, D=T
// Service_2, S2, T2=10, C2=1, D=T
// Service_3, S3, T3=15, C3=2, D=T
//
// Sequencer - 100 Hz [gives semaphores to all other services]
// Service_1 - 50 Hz, every other Sequencer loop
// Service_2 - 10 Hz, every 10th Sequencer loop 
// Service_3 - 6.67 Hz, every 15th Sequencer loop
//
// With the above, priorities by RM policy would be:
//
// Sequencer = RT_MAX	@ 100 Hz, T= 1
// Servcie_1 = RT_MAX-1	@ 50 Hz,  T= 2
// Service_2 = RT_MAX-2	@ 10 Hz,  T=10
// Service_3 = RT_MAX-3	@ 6.67 Hz T=15 
//
// Here are a few hardware/platform configuration settings
// that you should also check before running this code:
//
// 1) Check to ensure all your CPU cores on in an online state.
//
// 2) Check /sys/devices/system/cpu or do lscpu.
//
//    echo 1 > /sys/devices/system/cpu/cpu1/online
//    echo 1 > /sys/devices/system/cpu/cpu2/online
//    echo 1 > /sys/devices/system/cpu/cpu3/online
//
// 3) Check for precision time resolution and support with cat /proc/timer_list
//
// 4) Ideally all printf calls should be eliminated as they can interfere with
//    timing.  They should be replaced with an in-memory event logger or at
//    least calls to syslog.
//

// This is necessary for CPU affinity macros in Linux
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <semaphore.h>

#include <syslog.h>
#include <sys/time.h>
#include <errno.h>
#include "seqgen.h"
#include <sys/sysinfo.h>

#define ABS_DELAY
#define DRIFT_CONTROL
#define NUM_THREADS (3+1)

int abortTest=FALSE;
int abortS1=FALSE, abortS2=FALSE, abortS3=FALSE;
sem_t semS1, semS2, semS3;
static double start_time = 0;

pthread_t threads[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
pthread_attr_t main_attr;
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];



void main(void)
{
    double current_time;
    struct timespec rt_res, monotonic_res;
    int i, rc, cpuidx;
    cpu_set_t threadcpu;
    struct sched_param main_param;
    pid_t mainpid;

    start_time=getTimeMsec();

    // delay start for a second
    usleep(1000000);

    printf("Starting High Rate Sequencer Example\n");
    get_cpu_core_config();

    clock_getres(CLOCK_REALTIME, &rt_res);
    printf("RT clock resolution is %ld sec, %ld nsec\n", rt_res.tv_sec, rt_res.tv_nsec);

   printf("System has %d processors configured and %d available.\n", get_nprocs_conf(), get_nprocs());

    // initialize the sequencer semaphores
    //
    if (sem_init (&semS1, 0, 0))
        { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
    if (sem_init (&semS2, 0, 0)) 
        { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
    if (sem_init (&semS3, 0, 0)) 
        { printf ("Failed to initialize S3 semaphore\n"); exit (-1); }

    mainpid=getpid();

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    rc=sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    if(rc < 0) perror("main_param");

    print_scheduler();

    printf("rt_max_prio=%d\n", rt_max_prio);
    printf("rt_min_prio=%d\n", rt_min_prio);

    for(i=0; i < NUM_THREADS; i++)
    {

      CPU_ZERO(&threadcpu);
      cpuidx=(3);
      CPU_SET(cpuidx, &threadcpu);

      rc=pthread_attr_init(&rt_sched_attr[i]);
      rc=pthread_attr_setinheritsched(&rt_sched_attr[i], PTHREAD_EXPLICIT_SCHED);
      rc=pthread_attr_setschedpolicy(&rt_sched_attr[i], SCHED_FIFO);
      rc=pthread_attr_setaffinity_np(&rt_sched_attr[i], sizeof(cpu_set_t), &threadcpu);

      rt_param[i].sched_priority=rt_max_prio-i;
      pthread_attr_setschedparam(&rt_sched_attr[i], &rt_param[i]);

      threadParams[i].threadIdx=i;
    }
   
    printf("Service threads will run on %d CPU cores\n", CPU_COUNT(&threadcpu));

    current_time=getTimeMsec();
    syslog(LOG_CRIT, "RTMAIN: on cpu=%d @ sec=%lf, elapsed=%lf\n", sched_getcpu(), start_time, current_time);


    // Create Service threads which will block awaiting release for:
    //

    // Servcie_1 = RT_MAX-1	@ 50 Hz
    //
    rt_param[1].sched_priority=rt_max_prio-1;
    pthread_attr_setschedparam(&rt_sched_attr[1], &rt_param[1]);
    rc=pthread_create(&threads[1],               // pointer to thread descriptor
                      &rt_sched_attr[1],         // use specific attributes
                      //(void *)0,               // default attributes
                      Service_1,                 // thread function entry point
                      (void *)&(threadParams[1]) // parameters to pass in
                     );
    if(rc < 0)
        perror("pthread_create for service 1");
    else
        printf("pthread_create successful for service 1\n");


    // Service_2 = RT_MAX-2	@ 10 Hz
    //
    rt_param[2].sched_priority=rt_max_prio-2;
    pthread_attr_setschedparam(&rt_sched_attr[2], &rt_param[2]);
    rc=pthread_create(&threads[2], &rt_sched_attr[2], Service_2, (void *)&(threadParams[2]));
    if(rc < 0)
        perror("pthread_create for service 2");
    else
        printf("pthread_create successful for service 2\n");


    // Service_3 = RT_MAX-3	@ 6.67 Hz
    //
    rt_param[3].sched_priority=rt_max_prio-3;
    pthread_attr_setschedparam(&rt_sched_attr[3], &rt_param[3]);
    rc=pthread_create(&threads[3], &rt_sched_attr[3], Service_3, (void *)&(threadParams[3]));
    if(rc < 0)
        perror("pthread_create for service 3");
    else
        printf("pthread_create successful for service 3\n");


    // Create Sequencer thread, which like a cyclic executive, is highest prio
    printf("Start sequencer\n");
    threadParams[0].sequencePeriods=RTSEQ_PERIODS;

    // Sequencer = RT_MAX	@ 1000 Hz
    //
    rt_param[0].sched_priority=rt_max_prio;
    pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);
    rc=pthread_create(&threads[0], &rt_sched_attr[0], Sequencer, (void *)&(threadParams[0]));
    if(rc < 0)
        perror("pthread_create for sequencer service 0");
    else
        printf("pthread_create successful for sequeencer service 0\n");


   for(i=0;i<NUM_THREADS;i++)
       pthread_join(threads[i], NULL);

   printf("\nTEST COMPLETE\n");
}


void *Sequencer(void *threadp)
{
    struct timespec delay_time = {0, RTSEQ_DELAY_NSEC};
    struct timespec std_delay_time = {0, RTSEQ_DELAY_NSEC};
    struct timespec current_time_val={0,0};

    struct timespec remaining_time;
    double current_time, last_time, scaleDelay;
    double delta_t=(RTSEQ_DELAY_NSEC/(double)NANOSEC_PER_SEC);
    double scale_dt;
    int rc, delay_cnt=0;
    unsigned long long seqCnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    current_time=getTimeMsec(); last_time=current_time-delta_t;

    syslog(LOG_CRIT, "RTSEQ: start on cpu=%d @ sec=%lf after %lf with dt=%lf\n", sched_getcpu(), current_time, last_time, delta_t);

    do
    {
        current_time=getTimeMsec(); delay_cnt=0;

#ifdef DRIFT_CONTROL
        scale_dt = (current_time - last_time) - delta_t;
        delay_time.tv_nsec = std_delay_time.tv_nsec - (scale_dt * (NANOSEC_PER_SEC+DT_SCALING_UNCERTAINTY_NANOSEC))-CLOCK_BIAS_NANOSEC;
        //syslog(LOG_CRIT, "RTSEQ: scale dt=%lf @ sec=%lf after=%lf with dt=%lf\n", scale_dt, current_time, last_time, delta_t);
#else
        delay_time=std_delay_time; scale_dt=delta_t;
#endif


#ifdef ABS_DELAY
        clock_gettime(CLOCK_REALTIME, &current_time_val);
        delay_time.tv_sec = current_time_val.tv_sec;
        delay_time.tv_nsec = current_time_val.tv_nsec + delay_time.tv_nsec;

        if(delay_time.tv_nsec > NANOSEC_PER_SEC)
        {
            delay_time.tv_sec = delay_time.tv_sec + 1;
            delay_time.tv_nsec = delay_time.tv_nsec - NANOSEC_PER_SEC;
        }
        //syslog(LOG_CRIT, "RTSEQ: cycle %08llu delay for dt=%lf @ sec=%d, nsec=%d to sec=%d, nsec=%d\n", seqCnt, scale_dt, current_time_val.tv_sec, current_time_val.tv_nsec, delay_time.tv_sec, delay_time.tv_nsec);
#endif


        // Delay loop with check for early wake-up
        do
        {
#ifdef ABS_DELAY
            rc=clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &delay_time, (struct timespec *)0);
#else
            rc=clock_nanosleep(CLOCK_REALTIME, 0, &delay_time, &remaining_time);
#endif

            if(rc == EINTR)
            { 
                syslog(LOG_CRIT, "RTSEQ: EINTR @ sec=%lf\n", current_time);
                delay_cnt++;
            }
            else if(rc < 0)
            {
                perror("RTSEQ: nanosleep");
                exit(-1);
            }

            //syslog(LOG_CRIT, "RTSEQ: WOKE UP\n");
           
        } while(rc == EINTR);


        syslog(LOG_CRIT, "RTSEQ: cycle %08llu @ sec=%lf, last=%lf, dt=%lf, sdt=%lf\n", seqCnt, current_time, last_time, (current_time-last_time), scale_dt);

        // Release each service at a sub-rate of the generic sequencer rate

        // Servcie_1 = RT_MAX-1	@ 50 Hz
        if((seqCnt % 2) == 0) sem_post(&semS1);

        // Service_2 = RT_MAX-2	@ 10 Hz
        if((seqCnt % 10) == 0) sem_post(&semS2);

        // Service_3 = RT_MAX-3	@ 6.67 Hz
        if((seqCnt % 15) == 0) sem_post(&semS3);

        seqCnt++;
        last_time=current_time;

    } while(!abortTest && (seqCnt < threadParams->sequencePeriods));

    sem_post(&semS1); sem_post(&semS2); sem_post(&semS3);
    abortS1=TRUE; abortS2=TRUE; abortS3=TRUE;

    pthread_exit((void *)0);
}



void *Service_1(void *threadp)
{
    double current_time;
    unsigned long long S1Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    current_time=getTimeMsec();
    //syslog(LOG_CRIT, "S1: start on cpu=%d @ sec=%lf\n", sched_getcpu(), current_time);

    while(!abortS1)
    {
        sem_wait(&semS1);
        S1Cnt++;

        current_time=getTimeMsec();
        syslog(LOG_CRIT, "S1: release %llu @ sec=%lf\n", S1Cnt, current_time);
    }

    pthread_exit((void *)0);
}


void *Service_2(void *threadp)
{
    double current_time;
    unsigned long long S2Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    current_time=getTimeMsec();
    //syslog(LOG_CRIT, "S2: start on cpu=%d @ sec=%lf\n", sched_getcpu(), current_time);

    while(!abortS2)
    {
        sem_wait(&semS2);
        S2Cnt++;

        current_time=getTimeMsec();
        syslog(LOG_CRIT, "S2: release %llu @ sec=%lf\n", S2Cnt, current_time);
    }

    pthread_exit((void *)0);
}


void *Service_3(void *threadp)
{
    double current_time;
    unsigned long long S3Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    current_time=getTimeMsec();
    //syslog(LOG_CRIT, "S3: start on cpu=%d @ sec=%lf\n", sched_getcpu(), current_time);

    while(!abortS3)
    {
        sem_wait(&semS3);
        S3Cnt++;

        current_time=getTimeMsec();
        syslog(LOG_CRIT, "S3: release %llu @ sec=%lf\n", S3Cnt, current_time);
    }

    pthread_exit((void *)0);
}


// global start_time must be set on first call
double getTimeMsec(void)
{
  struct timespec event_ts = {0, 0};
  double event_time=0;

  clock_gettime(CLOCK_REALTIME, &event_ts);
  event_time = ((event_ts.tv_sec) + ((event_ts.tv_nsec)/(double)NANOSEC_PER_SEC));
  return (event_time - start_time);
}


void print_scheduler(void)
{
   int schedType, scope;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
       case SCHED_FIFO:
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
       case SCHED_OTHER:
           printf("Pthread Policy is SCHED_OTHER\n"); exit(-1);
         break;
       case SCHED_RR:
           printf("Pthread Policy is SCHED_RR\n"); exit(-1);
           break;
       default:
           printf("Pthread Policy is UNKNOWN\n"); exit(-1);
   }

   pthread_attr_getscope(&main_attr, &scope);

   if(scope == PTHREAD_SCOPE_SYSTEM)
       printf("PTHREAD SCOPE SYSTEM\n");
   else if (scope == PTHREAD_SCOPE_PROCESS)
       printf("PTHREAD SCOPE PROCESS\n");
   else
       printf("PTHREAD SCOPE UNKNOWN\n");
}


void get_cpu_core_config(void)
{
   cpu_set_t cpuset;
   pthread_t callingThread;
   int rc, idx;

   CPU_ZERO(&cpuset);

   // get affinity set for main thread
   callingThread = pthread_self();

   // Check the affinity mask assigned to the thread 
   rc = pthread_getaffinity_np(callingThread, sizeof(cpu_set_t), &cpuset);
   if (rc != 0)
       perror("pthread_getaffinity_np");
   else
   {
       printf("thread running on CPU=%d, CPUs =", sched_getcpu());

       for (idx = 0; idx < CPU_SETSIZE; idx++)
           if (CPU_ISSET(idx, &cpuset))
               printf(" %d", idx);

       printf("\n");
   }

   printf("Using CPUS=%d from total available.\n", CPU_COUNT(&cpuset));
}

