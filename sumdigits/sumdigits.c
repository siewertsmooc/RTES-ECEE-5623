#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define COUNT  (1000)

// Note that this is hardcoded to work for just 2 threads below
// Be very carefult about assuming you can just bump this up to make
// everythign work for a larger scale (number of workers).
#define NUM_THREADS (10)

// Note that often the "digit sum" rather than "sum of the digits" is defined as the sum of the digit in each 10's place, but
// that's not what we want to model here.  E.g., Wikipedia - https://en.wikipedia.org/wiki/Digit_sum
//
// What we want to model is an arithmetic series sum or "sum of numbers in range",  best referred to as the "sum or an arithmetic progression":
// * https://en.wikipedia.org/wiki/Arithmetic_progression
//
// This is what we mean summing numbers in the range 1...n, where we know based on series facts that sum(1...n) = n(n+1)/2
//
// We can now have threads sum sub-ranges of a series as a service and then have them add up  the result after a join so that
// sum(1...n) = sum(1...n/2-1) + sum(n/2...n-1) for example.
//
// This sample code provides a simple example of an arithmetic progression sum (sometimes called sum of the digits for simplicity since
// we know sum(0...9)=9(10)/2=45.
//
// For the 2 threaded example here, if we sum(0...20)=20(21)/2=210
// Thread[0]=sum( 0... 9)=9(10)/2=45
// Thread[1]=sum(10...19)=       175 -- note that this is 19(20)/2 - 9(10)/2 
// gsum=(0...19)=19(20)/2=190+20=210
//
// It should techically be called a sum of a series of numbers in an arithmetic progression.
//

typedef struct
{
    int threadIdx;
    int start;
    int end;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];

// Thread specific globals
int gsum[NUM_THREADS];

void *sumThread(void *threadp)
{
    int i, idx, start, end;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    start = threadParams->start;
    end = threadParams->end;
    idx = threadParams->threadIdx;

    printf("Thread %d summing range %d to %d\n", idx, start, end);

    for(i=start; i<=end; i++)
    {
        gsum[idx] = gsum[idx] + i;
        printf("thread idx=%d, gsum=%d\n", idx, gsum[idx]);
    }
}

int main (int argc, char *argv[])
{
   int range=COUNT/NUM_THREADS, gsumall=0; int i=0;

   // initialize gsum array to zero
   for(i=0; i<NUM_THREADS; i++)
       gsum[i]=0;

   printf("Each thread subrange is %d\n", range);

   for(i=0; i<NUM_THREADS; i++)
   {
      threadParams[i].threadIdx=i;
      threadParams[i].start=(i*range);
      threadParams[i].end=((i*range)+range)-1;

      pthread_create(&threads[i], (void *)0, sumThread, (void *)&(threadParams[i]));
   }

   for(i=0; i<NUM_THREADS; i++)
     pthread_join(threads[i], NULL);

   // we add int COUNT here in the final reduction since each worker
   // summed up to n-1, so we have to add final value n
   for(i=0; i<NUM_THREADS; i++)
       gsumall+=gsum[i];

   gsumall+=COUNT;

   printf("TEST COMPLETE: gsum[0]=%d, gsum[1]=%d, gsumall=%d\n", 
          gsum[0], gsum[1], gsumall);

   // Verfiy that sum of thread indexed sums is (n*(n+1))/2
   printf("TEST COMPLETE: gsumall=%d, [n[n+1]]/2=%d\n", gsumall, (COUNT*(COUNT+1))/2);
}
