#include <stdio.h>
#include <unistd.h>

// On x86 Pentium machines which implement the Time Stamp
// Counter in the PMU, pass in -DPMU_ANALYSIS.  For the Pentium,
// CPU cycles will be measured and CPI estimated based upon known
// instruction count.
//
// Leave the #define LONG_LONG_OK if your compiler and architecture
// support 64-bit unsigned integers, declared as unsigned long long in
// ANSI C.
//
// If not, please remove the #define below for 32-bit unsigned
// long declarations.
//


#define LONG_LONG_OK
#define FIB_LIMIT_FOR_32_BIT 47

typedef unsigned int UINT32;

#ifdef LONG_LONG_OK

typedef unsigned long long int UINT64;

UINT64 startTSC = 0;
UINT64 stopTSC = 0;
UINT64 cycleCnt = 0;

UINT64 readTSC(void)
{
   UINT64 ts;

   __asm__ volatile(".byte 0x0f,0x31" : "=A" (ts));
   return ts;
}

UINT64 cyclesElapsed(UINT64 stopTS, UINT64 startTS)
{
   return (stopTS - startTS);
}

#else

typedef struct
{
   UINT32 low;
   UINT32 high;
} TS64;

TS64 startTSC = {0, 0};
TS64 stopTSC = {0, 0};
TS64 cycleCnt = 0;

TS64 readTSC(void)
{
   TS64 ts;
   __asm__ volatile(".byte 0x0f,0x31" : "=a" (ts.low), "=d" (ts.high));
   return ts;
}

TS64 cyclesElapsed(TS64 stopTS, TS64 startTS)
{
   UINT32 overFlowCnt;
   UINT32 cycleCnt;
   TS64 elapsedT;

   overFlowCnt = (stopTSC.high - startTSC.high);

   if(overFlowCnt && (stopTSC.low < startTSC.low))
   {
      overFlowCnt--;
      cycleCnt = (0xffffffff - startTSC.low) + stopTSC.low;
   }
   else
   {
      cycleCnt = stopTSC.low - startTSC.low;
   }

   elapsedT.low = cycleCnt;
   elapsedT.high = overFlowCnt;

   return elapsedT;
}

#endif


UINT32 idx = 0, jdx = 1;
UINT32 seqIterations = FIB_LIMIT_FOR_32_BIT;
UINT32 reqIterations = 1, Iterations = 1;
UINT32 fib = 0, fib0 = 0, fib1 = 1;

#define FIB_TEST(seqCnt, iterCnt)      \
   for(idx=0; idx < iterCnt; idx++)    \
   {                                   \
      fib = fib0 + fib1;               \
      while(jdx < seqCnt)              \
      {                                \
         fib0 = fib1;                  \
         fib1 = fib;                   \
         fib = fib0 + fib1;            \
         jdx++;                        \
      }                                \
   }                                   \


void fib_wrapper(void)
{
   FIB_TEST(seqIterations, Iterations);
}


#define INST_CNT_FIB_INNER 15
#define INST_CNT_FIB_OUTTER 6
 

int main( int argc, char *argv[] )
{
   double clkRate = 0.0, fibCPI = 0.0;
   UINT32 instCnt = 0;

   if(argc == 2)
   {
      sscanf(argv[1], "%ld", &reqIterations);

      seqIterations = reqIterations % FIB_LIMIT_FOR_32_BIT;
      Iterations = reqIterations / seqIterations;
   }
   else if(argc == 1)
      printf("Using defaults\n");
   else
      printf("Usage: fibtest [Num iterations]\n");


   instCnt = (INST_CNT_FIB_INNER * seqIterations) +
             (INST_CNT_FIB_OUTTER * Iterations) + 1;

   // Estimate CPU clock rate
   startTSC = readTSC();
   usleep(1000000);
   stopTSC = readTSC();
   cycleCnt = cyclesElapsed(stopTSC, startTSC);

#ifdef LONG_LONG_OK
   printf("Cycle Count=%llu\n", cycleCnt);
   clkRate = ((double)cycleCnt)/1000000.0;
   printf("Based on usleep accuracy, CPU clk rate = %lu clks/sec,",
          cycleCnt);
   printf(" %7.1f Mhz\n", clkRate);
#else
   printf("Cycle Count=%lu\n", cycleCnt.low);
   printf("OverFlow Count=%lu\n", cycleCnt.high);
   clkRate = ((double)cycleCnt.low)/1000000.0;
   printf("Based on usleep accuracy, CPU clk rate = %lu clks/sec,",
          cycleCnt.low);
   printf(" %7.1f Mhz\n", clkRate);
#endif

   printf("\nRunning Fibonacci(%d) Test for %ld iterations\n",
          seqIterations, Iterations);


   // START Timed Fibonacci Test
   startTSC = readTSC();
   FIB_TEST(seqIterations, Iterations);
   stopTSC = readTSC();
   // END Timed Fibonacci Test


#ifdef LONG_LONG_OK
   printf("startTSC =0x%016x\n", startTSC);
   printf("stopTSC =0x%016x\n", stopTSC);

   cycleCnt = cyclesElapsed(stopTSC, startTSC);
   printf("\nFibonacci(%lu)=%lu (0x%08lx)\n", seqIterations, fib, fib);
   printf("\nCycle Count=%llu\n", cycleCnt);
   printf("\nInst Count=%lu\n", instCnt);
   fibCPI = ((double)cycleCnt) / ((double)instCnt);
   printf("\nCPI=%4.2f\n", fibCPI);

#else
   printf("startTSC high=0x%08x, startTSC low=0x%08x\n", startTSC.high, startTSC.low);
   printf("stopTSC high=0x%08x, stopTSC low=0x%08x\n", stopTSC.high, stopTSC.low);

   cycleCnt = cyclesElapsed(stopTSC, startTSC);
   printf("\nFibonacci(%lu)=%lu (0x%08lx)\n", seqIterations, fib, fib);
   printf("\nCycle Count=%lu\n", cycleCnt.low);
   printf("OverFlow Count=%lu\n", cycleCnt.high);
   fibCPI = ((double)cycleCnt.low) / ((double)instCnt);
   printf("\nCPI=%4.2f\n", fibCPI);

#endif

}
