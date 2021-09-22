#include "vxWorks.h"

#include "drv/timer/timerDev.h"

double jiffies_per_tick;
double clock_frequency;
double microseconds_per_tick;
double microseconds_per_jiffy;
double tick_diff, jiffy_diff, micro_diff;

unsigned long last_jiffies = 0;
unsigned long last_ticks = 0;
unsigned long current_jiffies = 0;
unsigned long current_ticks = 0;

void time_stamp(void)
{


 /* Enable the time-stamp driver for on-line collection */
  sysTimestampEnable();

  
  jiffies_per_tick = (double)sysTimestampPeriod();
  clock_frequency = (double)sysTimestampFreq();

  microseconds_per_tick = (jiffies_per_tick / clock_frequency)*1000000.0;
  microseconds_per_jiffy = microseconds_per_tick / jiffies_per_tick;

  printf("microseconds_per_tick = %e, microseconds_per_jiffy = %e\n", microseconds_per_tick, microseconds_per_jiffy);

  last_jiffies = sysTimestampLock();
  last_ticks = tickGet();

  taskDelay(100);

  current_jiffies = sysTimestampLock();
  current_ticks = tickGet();

  tick_diff = ((double)current_ticks - (double)last_ticks)*microseconds_per_tick;
  
  jiffy_diff = ((double)current_jiffies - (double)last_jiffies)*microseconds_per_jiffy;
  
  micro_diff = tick_diff + jiffy_diff;

  printf("micro_diff = %lf\n", micro_diff);

}
