/****************************************************************************/
/* Function: Basic POSIX message queue demo                                 */
/*                                                                          */
/* Sam Siewert, 02/05/2011                                                  */
/****************************************************************************/

//   Mounting the message queue file system
//       On  Linux,  message queues are created in a virtual file system.
//       (Other implementations may also provide such a feature, but  the
//       details  are likely to differ.)  This file system can be mounted
//       (by the superuser) using the following commands:

//           # mkdir /dev/mqueue
//           # mount -t mqueue none /dev/mqueue


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SNDRCV_MQ "/send_receive_mq"
#define MAX_MSG_SIZE 128
#define ERROR (-1)

struct mq_attr mq_attr;

pthread_t th_receive, th_send; // create threads
pthread_attr_t attr_receive, attr_send;
struct sched_param param_receive, param_send;

static char canned_msg[] = "this is a test, and only a test, in the event of a real emergency, you would be instructed ...";

void *receiver(void *arg)
{
  (void)arg;

  mqd_t mymq;
  char buffer[MAX_MSG_SIZE];
  unsigned int prio;
  int nbytes;

  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, S_IRWXU, &mq_attr);

  if(mymq == (mqd_t)ERROR)
  {
    perror("receiver mq_open");
    exit(-1);
  }
  else
  {
    printf("receviver opened mq\n");
  }

  /* read oldest, highest priority msg from the message queue */
  if((nbytes = mq_receive(mymq, buffer, MAX_MSG_SIZE, &prio)) == ERROR)
  {
    perror("mq_receive");
  }
  else
  {
    buffer[nbytes] = '\0';
    printf("receive: msg %s received with priority = %d, length = %d\n",
           buffer, prio, nbytes);
  }
    
  return NULL;
}

void *sender(void *arg)
{
  (void)arg;

  mqd_t mymq;
  //unsigned int prio;
  int nbytes;

  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, S_IRWXU, &mq_attr);

  if(mymq < 0)
  {
    perror("sender mq_open");
    exit(-1);
  }
  else
  {
    printf("sender opened mq\n");
  }

  /* send message with priority=30 */
  if((nbytes = mq_send(mymq, canned_msg, sizeof(canned_msg), 30)) == ERROR)
  {
    perror("mq_send");
  }
  else
  {
    printf("send: message successfully sent\n");
  }
  
  return NULL;
}

int main(void)
{
  int rc, rt_max_prio;

  mq_unlink(SNDRCV_MQ);

  /* setup common message q attributes */
  mq_attr.mq_maxmsg = 10;
  mq_attr.mq_msgsize = MAX_MSG_SIZE;
  mq_attr.mq_flags = 0;

  // Create two communicating processes right here
  // sender();
  // receiver();

  rt_max_prio = sched_get_priority_max(SCHED_FIFO);
  
  // Setup Receiver
  pthread_attr_init(&attr_receive);
  pthread_attr_setinheritsched(&attr_receive, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&attr_receive, SCHED_FIFO); 
  param_receive.sched_priority = rt_max_prio-1;
  pthread_attr_setschedparam(&attr_receive, &param_receive);
  
  // Setup Sender
  pthread_attr_init(&attr_send);
  pthread_attr_setinheritsched(&attr_send, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&attr_send, SCHED_FIFO); 
  param_send.sched_priority = rt_max_prio;
  pthread_attr_setschedparam(&attr_send, &param_send);
  
  if((rc=pthread_create(&th_send, &attr_send, sender, NULL)) == 0)
  {
    printf("Sender Thread Created\n");
  }
  else 
  {
    perror("Failed to Make Sender Thread\n");
    printf("rc=%d\n", rc);
    return EXIT_FAILURE;
  }

  if((rc=pthread_create(&th_receive, &attr_receive, receiver, NULL)) == 0)
  {
    printf("Receiver Thread Created\n");
  }
  else
  {
    perror("Failed Making Reciever Thread\n"); 
    printf("rc=%d\n", rc);
    return EXIT_FAILURE;
  }

  pthread_join(th_send, NULL);
  printf("Sender joined\n");  

  pthread_join(th_receive, NULL);
  printf("Reciever joined\n");

  return EXIT_SUCCESS;
}
