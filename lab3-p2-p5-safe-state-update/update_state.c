#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define NSEC_CONVERSION 1000000000

int stopProgram;

typedef struct
{
    double roll;
    double pitch;
    double yaw;
    double latitude;
    double longitude;
    double altitude_m;
    struct timespec timestamp;
} position_state_t;

position_state_t *state;

pthread_t stateUpdaterThread;
pthread_t stateReaderThread;

pthread_attr_t rt_thread_attr;
struct sched_param rt_sched_param;

pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

void printState()
{
    printf("*************************************************\n");
    double timestamp = (double)state->timestamp.tv_sec + (double)(state->timestamp.tv_nsec / (double)NSEC_CONVERSION);
    printf("Timestamp: %lf s\n", timestamp);
    printf("roll = %2.3f\tpitch = %2.3f\tyaw = %2.3f\n", state->roll, state->pitch, state->yaw);
    printf("Longitude = %2.3f\tLatitude = %2.3f\tAltitude in meters = %2.3f\n", state->longitude, state->latitude, state->altitude_m);
}

void initializeState()
{
    state = (position_state_t *)malloc(sizeof(position_state_t));
    if (NULL == state)
    {
        printf("Could not allocate memory for the state variable.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize state variable with values
    state->roll = ((double)rand());
    state->pitch = ((double)rand());
    state->yaw = ((double)rand());
    state->latitude = ((double)rand());
    state->longitude = ((double)rand());
    state->altitude_m = ((double)rand());

    clock_gettime(CLOCK_REALTIME, &(state->timestamp));
}

void *updateState(void *arg)
{
    while ( stopProgram == 0 )
    {
    pthread_mutex_lock(&state_mutex);

    // Update the state values and timestamp
    state->roll = ((double)rand());
    state->pitch = ((double)rand());
    state->yaw = ((double)rand());
    state->latitude = ((double)rand());
    state->longitude = ((double)rand());
    state->altitude_m = ((double)rand());

    clock_gettime(CLOCK_REALTIME, &(state->timestamp));
    pthread_mutex_unlock(&state_mutex);
    sleep(1);
    }
}

void *readState(void *arg)
{
    while ( stopProgram == 0 )
    {
        pthread_mutex_lock(&state_mutex);
        printState();
        pthread_mutex_unlock(&state_mutex);
        sleep(1);
    }
}

static void sigintHandler(int sig)
{
    if (sig == SIGINT)
    {
        if (state != NULL)
        {
            printf("\nFreeing allocated memory.");
            free(state);
        }
        stopProgram = 1;
        printf("\nProgram Stopping!\n");
    }
}

int main(int argc, char *argv[])
{
    stopProgram = 0;

    // Set up the handler for cleanup when the user hits ctrl+C
    signal(SIGINT, sigintHandler);

    srand(time(NULL));
    initializeState();
    printState();

    pthread_attr_init( &rt_thread_attr );
    pthread_attr_setinheritsched( &rt_thread_attr, PTHREAD_EXPLICIT_SCHED );
    pthread_attr_setschedpolicy( &rt_thread_attr, SCHED_FIFO );
    rt_sched_param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
    pthread_attr_setschedparam( &rt_thread_attr, &rt_sched_param );

    pthread_create( &stateUpdaterThread, &rt_thread_attr, updateState, NULL);
    pthread_create( &stateReaderThread, &rt_thread_attr, readState, NULL);

    pthread_join(stateUpdaterThread, NULL);
    pthread_join(stateReaderThread, NULL);

    pthread_mutex_destroy( &state_mutex );

    if (state != NULL)
    {
        free(state);
    }
    exit(EXIT_SUCCESS);
}
