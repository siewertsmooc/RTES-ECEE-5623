#include <syslog.h>
//#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#include <sched.h>

// for getpid()
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>

// Note that all syslog output goes to /var/log/syslog
//
// To view the output as your RT applicaiton is running, you can just tail the log as follows:
//
// tail -f /var/log/syslog
//
// tail -f /var/log/messages on some systems depending on configuration
//
// Syslog is more efficient than printf and causes less potential for "blocking" in the middle of your
// service execution.  Please use it instead of printf and use it because it is simpler and easier to use
// than ftrace (although ftrace is ideal, it is harder to learn and master).
//
// https://elinux.org/Ftrace
//
// To extract specific SYSLOGTRC messages only, use:
//
// cat /var/log/syslog | grep SYSLOGTRC > mytrace.txt
//
// So, syslog is a nice compromise between efficent software-in-circuit tracing and printf.
//
// Replaced out of date gettimeofday with POSIX clock_gettime.
//
// Clear your syslog as ROOT with: root@raspberrypi:/home/pi# > /var/log/syslog
//
// Compile with gcc syslogtest.c -o syslogtest -lrt
//

#define DURATION_SEC (10.0)

void log_scheduler(void);
pthread_attr_t main_attr;


int main(void)
{
    struct timespec current_time;
    //struct timeval tv;

    double current_ftime;
    double start_ftime;
    double last_ftime;

    // scheduler declarations
    struct sched_param main_param;
    int rt_max_prio, rt_min_prio, rc;

    // get the pid with getpid call
    pid_t mainpid=getpid();

    clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);
    start_ftime = (double)current_time.tv_sec + ((double)current_time.tv_nsec / 1000000000.0);

    last_ftime = start_ftime;

    // For duration, just log time from the source clock so we can plot and analyze using DEFAULT scheduler policy
    //
    do
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);
        current_ftime = (double)current_time.tv_sec + ((double)current_time.tv_nsec / 1000000000.0);

        //gettimeofday(&tv, (struct timezone *)0);
        syslog(LOG_CRIT, "SYSLOG_NRT_TRC: My log message test @ tv.tv_sec %ld, tv.tv_nsec %ld, realtime %6.9lf, dt=%6.9lf\n", 
    	       current_time.tv_sec, current_time.tv_nsec, (current_ftime - start_ftime), (current_ftime - last_ftime));

        last_ftime = current_ftime;

        usleep(1000000);

    } while ( (current_ftime - start_ftime) < DURATION_SEC);



    // Change scheduler for main thread from default SCHED_OTHER to SCHED_FIFO to see if this improves
    // the monotonicity of the logging

    log_scheduler();

    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    syslog(LOG_CRIT, "SYSLOGSCHED: rt_max_prio=%d\n", rt_max_prio);
    syslog(LOG_CRIT, "SYSLOGSCHED: rt_min_prio=%d\n", rt_min_prio);

    rc=sched_getparam(mainpid, &main_param);
    main_param.sched_priority=rt_max_prio;

    if((rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param)) < 0)
    {
        perror("******** WARNING: sched_setscheduler");
    }

    log_scheduler();


    // second logging test with new scheduler policy SCHED_FIFO
    //
    start_ftime = (double)current_time.tv_sec + ((double)current_time.tv_nsec / 1000000000.0);

    // For duration, just log time from the source clock so we can plot and analyze
    do
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);
        current_ftime = (double)current_time.tv_sec + ((double)current_time.tv_nsec / 1000000000.0);
        //gettimeofday(&tv, (struct timezone *)0);
        syslog(LOG_CRIT, "SYSLOG_RT_TRC: My log message test @ tv.tv_sec %ld, tv.tv_nsec %ld, realtime %6.9lf, dt=%6.9lf\n", 
    	       current_time.tv_sec, current_time.tv_nsec, (current_ftime - start_ftime), (current_ftime - last_ftime));

        last_ftime = current_ftime;

        usleep(1000000);

    } while ( (current_ftime - start_ftime) < DURATION_SEC);


}


void log_scheduler(void)
{
   int schedType, scope;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
           syslog(LOG_CRIT, "SYSLOGSCHED: Pthread Policy is SCHED_FIFO\n");
           printf("SYSLOGSCHED: Pthread Policy is SCHED_FIFO\n");
           break;
     case SCHED_OTHER:
           syslog(LOG_CRIT, "SYSLOGSCHED: Pthread Policy is SCHED_OTHER\n");
           printf("SYSLOGSCHED: Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:
           syslog(LOG_CRIT, "SYSLOGSCHED: Pthread Policy is SCHED_OTHER\n");
           printf("SYSLOGSCHED: Pthread Policy is SCHED_OTHER\n");
           break;
     default:
       syslog(LOG_CRIT, "SYSLOGSCHED: Pthread Policy is UNKNOWN\n");
   }

   pthread_attr_getscope(&main_attr, &scope);

   if(scope == PTHREAD_SCOPE_SYSTEM)
   {
     syslog(LOG_CRIT, "SYSLOGSCHED: PTHREAD SCOPE SYSTEM\n");
     printf("SYSLOGSCHED: PTHREAD SCOPE SYSTEM\n");
   }
   else if (scope == PTHREAD_SCOPE_PROCESS)
   {
     syslog(LOG_CRIT, "SYSLOGSCHED: PTHREAD SCOPE PROCESS\n");
     printf("SYSLOGSCHED: PTHREAD SCOPE PROCESS\n");
   }
   else
   {
     syslog(LOG_CRIT, "SYSLOGSCHED: PTHREAD SCOPE UNKNOWN\n");
     printf("SYSLOGSCHED: PTHREAD SCOPE UNKNOWN\n");
   }

}

