/* clock_times.c
 *
 * from the man page - https://man7.org/linux/man-pages/man2/clock_getres.2.html
 *
 *  Licensed under GNU General Public License v2 or later.
*/
#define _XOPEN_SOURCE 600
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define SECS_IN_DAY (24 * 60 * 60)

static void
displayClock(clockid_t clock, char *name, bool showRes)
{
    struct timespec ts;

    if (clock_gettime(clock, &ts) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }

    printf("%-15s: %10ld.%03ld (", name,
            (long) ts.tv_sec, ts.tv_nsec / 1000000);

    long days = ts.tv_sec / SECS_IN_DAY;
    if (days > 0)
        printf("%ld days + ", days);

    printf("%2ldh %2ldm %2lds", (ts.tv_sec % SECS_IN_DAY) / 3600,
            (ts.tv_sec % 3600) / 60, ts.tv_sec % 60);
    printf(")\n");

    if (clock_getres(clock, &ts) == -1) {
        perror("clock_getres");
        exit(EXIT_FAILURE);
    }

    if (showRes)
        printf("     resolution: %10ld.%09ld\n",
                (long) ts.tv_sec, ts.tv_nsec);
}

#define DELAY_LOOPS (10)
#define USEC_PER_MSEC (1000)

int main(int argc, char *argv[])
{
    struct timespec delay_time={0,10000000};
    struct timespec remaining_time={0,1000000};
    bool showRes = argc > 1;
    int idx=0, rc;

    displayClock(CLOCK_REALTIME, "CLOCK_REALTIME", showRes);
#ifdef CLOCK_TAI
    displayClock(CLOCK_TAI, "CLOCK_TAI", showRes);
#endif
    displayClock(CLOCK_MONOTONIC, "CLOCK_MONOTONIC", showRes);
#ifdef CLOCK_BOOTTIME
    displayClock(CLOCK_BOOTTIME, "CLOCK_BOOTTIME", showRes);
#endif

    printf("Iteration tests\n");

    for(idx=0; idx < DELAY_LOOPS; idx++)
    {
        printf("idx=%d ", idx); displayClock(CLOCK_REALTIME, "CLOCK_REALTIME", showRes);
        if(rc=nanosleep(&delay_time, &remaining_time) == 0)
            displayClock(CLOCK_REALTIME, "CLOCK_REALTIME", showRes);
	else {perror("nanosleep"); exit(-1);};
    }

    printf("\n");

    for(idx=0; idx < DELAY_LOOPS; idx++)
    {
        printf("idx=%d ", idx); displayClock(CLOCK_MONOTONIC, "CLOCK_MONOTONIC", showRes);
        if(rc=nanosleep(&delay_time, &remaining_time) == 0)
            displayClock(CLOCK_MONOTONIC, "CLOCK_MONOTONIC", showRes);
	else {perror("nanosleep"); exit(-1);};
    }

    printf("\n");

    for(idx=0; idx < DELAY_LOOPS; idx++)
    {
        printf("idx=%d ", idx); displayClock(CLOCK_MONOTONIC_RAW, "CLOCK_MONOTONIC_RAW", showRes);
        if(rc=nanosleep(&delay_time, &remaining_time) == 0)
            displayClock(CLOCK_MONOTONIC_RAW, "CLOCK_MONOTONIC_RAW", showRes);
	else {perror("nanosleep"); exit(-1);};
    }


    exit(EXIT_SUCCESS);
}
