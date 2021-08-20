#include <linux/module.h>
#include <linux/kernel.h>

/*
 * works for ARM1176JZ-F
 */

int init_module(void)
{
  asm volatile ("mcr p15,  0, %0, c15,  c9, 0\n" : : "r" (1)); 
  printk (KERN_INFO "User-level access to CCR has been turned on.\n");
  return 0;
}

void cleanup_module(void)
{
}
