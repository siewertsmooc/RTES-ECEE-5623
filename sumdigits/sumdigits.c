#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define COUNT  100

#define NUM_THREADS (3)

// Note that often the "digit sum" rather than "sum of the digits" or "sum of numbers in a sequence" is defined as the sum of the digit in each 10's place, but
// that's not what we want to model here.  E.g., Wikipedia - https://en.wikipedia.org/wiki/Digit_sum
//
// What we want to model is an arithmetic series sum best referred to as the "sum or an arithmetic progression" - https://en.wikipedia.org/wiki/Arithmetic_progression
//
// This si what we mean summing numbers in the range 1...n, where we know based on series facts that sum(1...n) = n(n+1)/2
//
// We can now have threads sum sub-ranges of a series as a service and then have them add up  the result after a join so that sum(1...n) = sum(1...n/2-1) + sum(n/2...n-1) for
// example.
//
// This sample code provides a simple example of an arithmetic progression sum (sometimes called sum of the digits for simplicity since we know sum(0...9)=9(10)/2=45.
//
// It should techically be called a sum of a series of numbers in an arithmetic progression.
//

typedef struct
{
    int threadIdx;
} threadParams_t;


// POSIX thread declarations and scheduling attributes
pthread_t threads[3];
threadParams_t threadParams[3];

// Thread specific globals
int gsum[3]={0, 100, 200};

void *sumThread(void *threadp)
{
    int i, idx;
    threadParams_t *threadParams = (threadParams_t *)threadp;

    idx = threadParams->threadIdx;

    for(i=1; i<COUNT; i++)
    {
        gsum[idx] = gsum[idx] + (i + (idx*COUNT));
        printf("Increment %d for thread idx=%d, gsum=%d\n", i, idx, gsum[idx]);
    }
}

int main (int argc, char *argv[])
{
   int rc, testsum=0; int i=0;

   for(i=0; i<3; i++)
   {
      threadParams[i].threadIdx=i;
      pthread_create(&threads[i], (void *)0, sumThread, (void *)&(threadParams[i]));
   }

   for(i=0; i<NUM_THREADS; i++)
     pthread_join(threads[i], NULL);

   printf("TEST COMPLETE: gsum[0]=%d, gsum[1]=%d, gsum[2]=%d, gsumall=%d\n", 
          gsum[0], gsum[1], gsum[2], (gsum[0]+gsum[1]+gsum[2]));

   // Verfiy with single thread version and (n*(n+1))/2
   for(i=1; i<(3*COUNT); i++)
       testsum = testsum + i;

   printf("TEST COMPLETE: testsum=%d, [n[n+1]]/2=%d\n", testsum, ((3*COUNT-1)*(3*COUNT))/2);
}
