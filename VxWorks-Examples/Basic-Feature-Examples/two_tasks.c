/****************************************************************************/
/*                                                                          */
/* Sam Siewert - 2001                                                       */
/* Modified by Dan Walkes Fall 2005                                         */
/* Fixed problem with taskDelete on taskNameToId - Sam S.                   */
/*                                                                          */
/****************************************************************************/

#include "vxWorks.h"
#include "taskLib.h"
#include "semLib.h"
#include "sysLib.h"

SEM_ID synch_sem;                                                                   
int abort_test = FALSE;
int take_cnt = 0;
int give_cnt = 0;
 
void task_a(void)
{
  int cnt = 0;

  while(!abort_test)
  {
    taskDelay(1000);
    for(cnt=0;cnt < 10000000;cnt++);
    semGive(synch_sem); 
    give_cnt++;
  }
}

void task_b(void)
{
  int cnt = 0;

  while(!abort_test)
  {
    for(cnt=0;cnt < 10000000;cnt++);
    take_cnt++;
    semTake(synch_sem, WAIT_FOREVER);
    taskDelay(1000);
  }
}


void test_tasks1(void)
{
  int taskId;

  sysClkRateSet(1000);


  if(taskId = (taskNameToId("task_a") != ERROR))
    taskDelete(taskId);

  if(taskId = (taskNameToId("task_b") != ERROR))
    taskDelete(taskId);
  
  synch_sem = semBCreate(SEM_Q_FIFO, SEM_FULL);

  if(taskSpawn("task_a", 100, 0, 4000, task_a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
  {
    printf("Task A task spawn failed\n");
  }
  else
    printf("Task A task spawned\n");

  taskDelay(10);
  if(taskSpawn("task_b", 90, 0, 4000, task_b, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR) 
  {
    printf("Task B task spawn failed\n");
  }
  else
    printf("Task B task spawned\n");

}


void test_tasks2(void)
{
  int taskId;

  sysClkRateSet(1000);

  if(taskId = (taskNameToId("task_a") != ERROR))
    taskDelete(taskId);

  if(taskId = (taskNameToId("task_b") != ERROR))
    taskDelete(taskId);
  
  synch_sem = semBCreate(SEM_Q_FIFO, SEM_FULL);

  if(taskSpawn("task_a", 90, 0, 4000, task_a, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
  {
    printf("Task A task spawn failed\n");
  }
  else
    printf("Task A task spawned\n");

  taskDelay(10);

  if(taskSpawn("task_b", 100, 0, 4000, task_b, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR) 
  {
    printf("Task B task spawn failed\n");
  }
  else
    printf("Task B task spawned\n");

   
}
