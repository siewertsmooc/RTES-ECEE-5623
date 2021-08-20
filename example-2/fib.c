#include "fib.h"

unsigned int idx = 0, jdx = 1;
unsigned int fib = 0, fib0 = 0, fib1 = 1;
unsigned int seqIter=FIB_LIMIT_FOR_32_BIT, loopIter=1;

void wrapper(void)
{
  FIB_TEST(seqIter, loopIter);
}
