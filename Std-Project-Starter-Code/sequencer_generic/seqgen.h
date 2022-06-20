#ifndef _SEQGEN_

#define USEC_PER_MSEC (1000)
#define MSEC_PER_SEC (1000)
#define NANOSEC_PER_SEC (1000000000)
#define NUM_CPU_CORES (1)
#define TRUE (1)
#define FALSE (0)

#define RTSEQ_PERIODS (2400)

// increas this if you fall behind over time
// default is 100 microseconds
#define CLOCK_BIAS_NANOSEC (100000)

// sleep dt scaling uncertainty
// default is 1/2 millisecond
#define DT_SCALING_UNCERTAINTY_NANOSEC (500000)

// default to 10 millisecond, 100 Hz
#define RTSEQ_DELAY_NSEC 		(10000000)

// default to 1 millisecond, 1000 Hz
//#define RTSEQ_DELAY_NSEC 		( 1000000)

typedef struct
{
    int threadIdx;
    unsigned long long sequencePeriods;
} threadParams_t;


double getTimeMsec(void);
void print_scheduler(void);
void get_cpu_core_config(void);

void *Sequencer(void *threadp);

void *Service_1(void *threadp);
void *Service_2(void *threadp);
void *Service_3(void *threadp);
void *Service_4(void *threadp);
void *Service_5(void *threadp);
void *Service_6(void *threadp);
void *Service_7(void *threadp);
void *Service_8(void *threadp);

#endif
