#include "vxWorks.h"
#include "taskLib.h"
#include "taskHookLib.h"
#include "tickLib.h"

unsigned long int gv_switchcnt = 0;
unsigned long int gv_switchtime;


/* Warning: Only code which can be executed in kernel/interrupt context
            can be placed in a taskSwitchHook call-back!

            Please read the VxWorks Ref. Manual carefully before adding
            API calls to ensure that they may be called in this context.
*/

void updateCntHook( WIND_TCB *pOldTcb,    /* pointer to old task's WIND_TCB */
                    WIND_TCB *pNewTcb     /* pointer to new task's WIND_TCB */
                   )
{

  gv_switchcnt++;
  gv_switchtime = tickGet();
  
}


void testhook(void)
{
  if(taskSwitchHookAdd((FUNCPTR)updateCntHook) == ERROR)
    printf("Error adding hook!\n");
  else
    printf("Added hook!\n");

}

void unldhook(void)
{
  if(taskSwitchHookDelete((FUNCPTR)updateCntHook) == ERROR)
    printf("Error deleting hook!\n");
  else
    printf("Deleted hook!\n");

}
