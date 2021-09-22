#include "semLib.h"
#include "signal.h"
#include "stdlib.h"
#include "stdio.h"   
#include "taskLib.h"   
#include "pc.h"
#include "intLib.h"
#include "iv.h"    
#include "config.h"


int pit_int_cnt = 0;
int int_vec = -1;

void connect_interrupt(void);
void pit_isr(int ivec);

void connect_interrupt(void) 
{
    STATUS sts;

    printf("RTC_INT_VEC = 0x%x\n", RTC_INT_VEC);
    printf("TIMER_INT_VEC = 0x%x\n", TIMER_INT_VEC);
    printf("PIT0_INT_VEC = 0x%x\n", PIT0_INT_VEC);

    if((sts = intConnect(INUM_TO_IVEC(RTC_INT_VEC), (VOIDFUNCPTR)pit_isr, RTC_INT_VEC)) == ERROR)
        printf("************* Int Connect Error ******************\n");
    else
        printf("************* Int Connect OK *****************************\n");


    if((sts = intConnect(INUM_TO_IVEC(TIMER_INT_VEC), (VOIDFUNCPTR)pit_isr, TIMER_INT_VEC)) == ERROR)
        printf("************* Int Connect Error ******************\n");
    else
        printf("************* Int Connect OK *****************************\n");


    if((sts = intConnect(INUM_TO_IVEC(PIT0_INT_VEC), (VOIDFUNCPTR)pit_isr, PIT0_INT_VEC)) == ERROR)
        printf("************* Int Connect Error ******************\n");
    else
        printf("************* Int Connect OK *****************************\n");

}


void pit_isr(int ivec)  
{
  pit_int_cnt++;
  int_vec = ivec;

  tickAnnounce();
 
}


