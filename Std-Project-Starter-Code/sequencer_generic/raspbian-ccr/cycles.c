static inline unsigned ccnt_read (void)
{
  unsigned cc;
  __asm__ volatile ("mrc p15, 0, %0, c15, c12, 1":"=r" (cc));
  return cc;
}
