// Sam Siewert, December 2020
//
// Sequencer Generic Demonstration
//
// Sequencer - 100 Hz 
//                   [gives semaphores to all other services]
// Service_1 - 25 Hz, every 4th Sequencer loop reads a V4L2 video frame
// Service_2 -  1 Hz, every 100th Sequencer loop writes out the current video frame
//
// With the above, priorities by RM policy would be:
//
// Sequencer = RT_MAX	@ 100 Hz
// Servcie_1 = RT_MAX-1	@ 25  Hz
// Service_2 = RT_MIN	@ 1   Hz
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
#include <sys/sysinfo.h>
#include <errno.h>

#include <signal.h>

#define USEC_PER_MSEC (1000)
#define NANOSEC_PER_MSEC (1000000)
#define NANOSEC_PER_SEC (1000000000)
#define NUM_CPU_CORES (4)
#define TRUE (1)
#define FALSE (0)

#define RT_CORE (2)

#define NUM_THREADS (3)

// Of the available user space clocks, CLOCK_MONONTONIC_RAW is typically most precise and not subject to 
// updates from external timer adjustments
//
// However, some POSIX functions like clock_nanosleep can only use adjusted CLOCK_MONOTONIC or CLOCK_REALTIME
//
//#define MY_CLOCK_TYPE CLOCK_REALTIME
//#define MY_CLOCK_TYPE CLOCK_MONOTONIC
#define MY_CLOCK_TYPE CLOCK_MONOTONIC_RAW
//#define MY_CLOCK_TYPE CLOCK_REALTIME_COARSE
//#define MY_CLOCK_TYPE CLOCK_MONTONIC_COARSE

int abortTest=FALSE;
int abortS1=FALSE, abortS2=FALSE, abortS3=FALSE;
sem_t semS1, semS2, semS3;
struct timespec start_time_val;
double start_realtime;

static timer_t timer_1;
static struct itimerspec itime = {{1,0}, {1,0}};
static struct itimerspec last_itime;

static unsigned long long seqCnt=0;

typedef struct
{
    int threadIdx;
} threadParams_t;


void Sequencer(int id);

void *Service_1_frame_acquisition(void *threadp);
void *Service_2_frame_process(void *threadp);
void *Service_3_frame_storage(void *threadp);

int seq_frame_read(void);
int seq_frame_process(void);
int seq_frame_store(void);

double getTimeMsec(void);
double realtime(struct timespec *tsptr);
void print_scheduler(void);


int v4l2_frame_acquisition_initialization(char *dev_name);
int v4l2_frame_acquisition_shutdown(void);
int v4l2_frame_acquisition_loop(char *dev_name);


void main(void)
{
    struct timespec current_time_val, current_time_res;
    double current_realtime, current_realtime_res;

    char *dev_name="/dev/video0";

    int i, rc, scope, flags=0;

    cpu_set_t threadcpu;
    cpu_set_t allcpuset;

    pthread_t threads[NUM_THREADS];
    threadParams_t threadParams[NUM_THREADS];
    pthread_attr_t rt_sched_attr[NUM_THREADS];
    int rt_max_prio, rt_min_prio, cpuidx;

    struct sched_param rt_param[NUM_THREADS];
    struct sched_param main_param;

    pthread_attr_t main_attr;
    pid_t mainpid;

    v4l2_frame_acquisition_initialization(dev_name);

    // required to get camera initialized and ready
    seq_frame_read();

    printf("Starting High Rate Sequencer Demo\n");
    clock_gettime(MY_CLOCK_TYPE, &start_time_val); start_realtime=realtime(&start_time_val);
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    clock_getres(MY_CLOCK_TYPE, &current_time_res); current_realtime_res=realtime(&current_time_res);
    printf("START High Rate Sequencer @ sec=%6.9lf with resolution %6.9lf\n", (current_realtime - start_realtime), current_realtime_res);
    syslog(LOG_CRIT, "START High Rate Sequencer @ sec=%6.9lf with resolution %6.9lf\n", (current_realtime - start_realtime), current_realtime_res);


   printf("System has %d processors configured and %d available.\n", get_nprocs_conf(), get_nprocs());

   CPU_ZERO(&allcpuset);

   for(i=0; i < NUM_CPU_CORES; i++)
       CPU_SET(i, &allcpuset);

   printf("Using CPUS=%d from total available.\n", CPU_COUNT(&allcpuset));


    // initialize the sequencer semaphores
    //
    if (sem_init (&semS1, 0, 0)) { printf ("Failed to initialize S1 semaphore\n"); exit (-1); }
    if (sem_init (&semS2, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }
    if (sem_init (&semS3, 0, 0)) { printf ("Failed to initialize S2 semaphore\n"); exit (-1); }

    mainpid=getpid();

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    rc=sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;
    rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);
    if(rc < 0) perror("main_param");
    print_scheduler();


    pthread_attr_getscope(&main_attr, &scope);

    if(scope == PTHREAD_SCOPE_SYSTEM)
      printf("PTHREAD SCOPE SYSTEM\n");
    else if (scope == PTHREAD_SCOPE_PROCESS)
      printf("PTHREAD SCOPE PROCESS\n");
    else
      printf("PTHREAD SCOPE UNKNOWN\n");

    printf("rt_max_prio=%d\n", rt_max_prio);
    printf("rt_min_prio=%d\n", rt_min_prio);


    for(i=0; i < NUM_THREADS; i++)
    {

      // run ALL threads on core RT_CORE
      CPU_ZERO(&threadcpu);
      cpuidx=(RT_CORE);
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

    // Create Service threads which will block awaiting release for:
    //

    // Servcie_1 = RT_MAX-1	@ 25 Hz
    //
    rt_param[0].sched_priority=rt_max_prio-1;
    pthread_attr_setschedparam(&rt_sched_attr[0], &rt_param[0]);
    rc=pthread_create(&threads[0],               // pointer to thread descriptor
                      &rt_sched_attr[0],         // use specific attributes
                      //(void *)0,               // default attributes
                      Service_1_frame_acquisition,                 // thread function entry point
                      (void *)&(threadParams[0]) // parameters to pass in
                     );
    if(rc < 0)
        perror("pthread_create for service 1 - V4L2 video frame acquisition");
    else
        printf("pthread_create successful for service 1\n");


    // Service_2 = RT_MAX-2	@ 1 Hz
    //
    rt_param[1].sched_priority=rt_max_prio-2;
    pthread_attr_setschedparam(&rt_sched_attr[1], &rt_param[1]);
    rc=pthread_create(&threads[1], &rt_sched_attr[1], Service_2_frame_process, (void *)&(threadParams[1]));
    if(rc < 0)
        perror("pthread_create for service 2 - flash frame storage");
    else
        printf("pthread_create successful for service 2\n");


    // Service_3 = RT_MAX-3	@ 1 Hz
    //
    rt_param[2].sched_priority=rt_max_prio-3;
    pthread_attr_setschedparam(&rt_sched_attr[2], &rt_param[2]);
    rc=pthread_create(&threads[2], &rt_sched_attr[2], Service_3_frame_storage, (void *)&(threadParams[2]));
    if(rc < 0)
        perror("pthread_create for service 3 - flash frame storage");
    else
        printf("pthread_create successful for service 3\n");


    // Wait for service threads to initialize and await relese by sequencer.
    //
    // Note that the sleep is not necessary of RT service threads are created with 
    // correct POSIX SCHED_FIFO priorities compared to non-RT priority of this main
    // program.
    //
    // sleep(1);
 
    // Create Sequencer thread, which like a cyclic executive, is highest prio
    printf("Start sequencer\n");

    // Sequencer = RT_MAX	@ 100 Hz
    //
    /* set up to signal SIGALRM if timer expires */
    timer_create(CLOCK_REALTIME, NULL, &timer_1);

    signal(SIGALRM, (void(*)()) Sequencer);


    /* arm the interval timer */
    itime.it_interval.tv_sec = 0;
    itime.it_interval.tv_nsec = 10000000;
    itime.it_value.tv_sec = 0;
    itime.it_value.tv_nsec = 10000000;
    //itime.it_interval.tv_sec = 1;
    //itime.it_interval.tv_nsec = 0;
    //itime.it_value.tv_sec = 1;
    //itime.it_value.tv_nsec = 0;

    timer_settime(timer_1, flags, &itime, &last_itime);


    for(i=0;i<NUM_THREADS;i++)
    {
        if(rc=pthread_join(threads[i], NULL) < 0)
		perror("main pthread_join");
	else
		printf("joined thread %d\n", i);
    }

   v4l2_frame_acquisition_shutdown();

   printf("\nTEST COMPLETE\n");
}



void Sequencer(int id)
{
    struct timespec current_time_val;
    double current_realtime;
    int rc, flags=0;

    // received interval timer signal
    if(abortTest)
    {
        // disable interval timer
        itime.it_interval.tv_sec = 0;
        itime.it_interval.tv_nsec = 0;
        itime.it_value.tv_sec = 0;
        itime.it_value.tv_nsec = 0;
        timer_settime(timer_1, flags, &itime, &last_itime);
	printf("Disabling sequencer interval timer with abort=%d and %llu\n", abortTest, seqCnt);

	// shutdown all services
        abortS1=TRUE; abortS2=TRUE; abortS3=TRUE;
        sem_post(&semS1); sem_post(&semS2); sem_post(&semS3);

    }
           
    seqCnt++;

    //clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    //printf("Sequencer on core %d for cycle %llu @ sec=%6.9lf\n", sched_getcpu(), seqCnt, current_realtime-start_realtime);
    //syslog(LOG_CRIT, "Sequencer on core %d for cycle %llu @ sec=%6.9lf\n", sched_getcpu(), seqCnt, current_realtime-start_realtime);


    // Release each service at a sub-rate of the generic sequencer rate

    // Servcie_1 @ 5 Hz
    if((seqCnt % 20) == 0) sem_post(&semS1);

    // Service_2 @ 1 Hz
    if((seqCnt % 100) == 0) sem_post(&semS2);

    // Service_3 @ 1 Hz
    if((seqCnt % 100) == 0) sem_post(&semS3);
}




void *Service_1_frame_acquisition(void *threadp)
{
    struct timespec current_time_val;
    double current_realtime;
    unsigned long long S1Cnt=0;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    // Start up processing and resource initialization
    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S1 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
    printf("S1 thread @ sec=%6.9lf\n", current_realtime-start_realtime);

    while(!abortS1) // check for synchronous abort request
    {
	// wait for service request from the sequencer, a signal handler or ISR in kernel
        sem_wait(&semS1);

	if(abortS1) break;
        S1Cnt++;

	// DO WORK - acquire V4L2 frame here or OpenCV frame here
	seq_frame_read();

	// on order of up to milliseconds of latency to get time
        clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
        syslog(LOG_CRIT, "S1 at 25 Hz on core %d for release %llu @ sec=%6.9lf\n", sched_getcpu(), S1Cnt, current_realtime-start_realtime);

	if(S1Cnt > 250) {abortTest=TRUE;};
    }

    // Resource shutdown here
    //
    pthread_exit((void *)0);
}


void *Service_2_frame_process(void *threadp)
{
    struct timespec current_time_val;
    double current_realtime;
    unsigned long long S2Cnt=0;
    int process_cnt;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S2 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
    printf("S2 thread @ sec=%6.9lf\n", current_realtime-start_realtime);

    while(!abortS2)
    {
        sem_wait(&semS2);

	if(abortS2) break;
        S2Cnt++;

	// DO WORK - transform frame
	process_cnt=seq_frame_process();

        clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
        syslog(LOG_CRIT, "S2 at 1 Hz on core %d for release %llu @ sec=%6.9lf\n", sched_getcpu(), S2Cnt, current_realtime-start_realtime);
    }

    pthread_exit((void *)0);
}


void *Service_3_frame_storage(void *threadp)
{
    struct timespec current_time_val;
    double current_realtime;
    unsigned long long S3Cnt=0;
    int store_cnt;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
    syslog(LOG_CRIT, "S3 thread @ sec=%6.9lf\n", current_realtime-start_realtime);
    printf("S3 thread @ sec=%6.9lf\n", current_realtime-start_realtime);

    while(!abortS3)
    {
        sem_wait(&semS3);

	if(abortS3) break;
        S3Cnt++;

	// DO WORK - store frame
	store_cnt=seq_frame_store();

        clock_gettime(MY_CLOCK_TYPE, &current_time_val); current_realtime=realtime(&current_time_val);
        syslog(LOG_CRIT, "S3 at 1 Hz on core %d for release %llu @ sec=%6.9lf\n", sched_getcpu(), S3Cnt, current_realtime-start_realtime);

	// after last write, set synchronous abort
	if(store_cnt == 10) {abortTest=TRUE;};
    }

    pthread_exit((void *)0);
}



double getTimeMsec(void)
{
  struct timespec event_ts = {0, 0};

  clock_gettime(MY_CLOCK_TYPE, &event_ts);
  return ((event_ts.tv_sec)*1000.0) + ((event_ts.tv_nsec)/1000000.0);
}


double realtime(struct timespec *tsptr)
{
    return ((double)(tsptr->tv_sec) + (((double)tsptr->tv_nsec)/1000000000.0));
}


void print_scheduler(void)
{
   int schedType;

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
       //case SCHED_DEADLINE:
       //    printf("Pthread Policy is SCHED_DEADLINE\n"); exit(-1);
       //    break;
       default:
           printf("Pthread Policy is UNKNOWN\n"); exit(-1);
   }
}

