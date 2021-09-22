/****************************************************************************/
/* Function: Basic POSIX message queue demo from VxWorks Prog. Guide p. 78  */
/*                                                                          */
/* Sam Siewert - 9/24/97                                                    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
                                                                    
#include "msgQLib.h"
#include "mqueue.h"
#include "errnoLib.h" 
#include "ioLib.h" 

#define SNDRCV_MQ "send_receive_mq"
#define MAX_MSG_SIZE 128

struct mq_attr mq_attr;

void receiver(void)
{
  mqd_t mymq;
  char buffer[MAX_MSG_SIZE];
  int prio;
  int nbytes;

  /* note that VxWorks does not deal with permissions? */
  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, 0, &mq_attr);

  if(mymq == (mqd_t)ERROR)
    perror("mq_open");

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
    
}

static char canned_msg[] = "this is a test, and only a test, in the event of a real emergency, you would be instructed ...";

void sender(void)
{
  mqd_t mymq;
  int prio;
  int nbytes;

  /* note that VxWorks does not deal with permissions? */
  mymq = mq_open(SNDRCV_MQ, O_RDWR, 0, &mq_attr);

  if(mymq == (mqd_t)ERROR)
    perror("mq_open");

  /* send message with priority=30 */
  if((nbytes = mq_send(mymq, canned_msg, sizeof(canned_msg), 30)) == ERROR)
  {
    perror("mq_send");
  }
  else
  {
    printf("send: message successfully sent\n");
  }
  
}


void mq_demo(void)
{

  /* setup common message q attributes */
  mq_attr.mq_maxmsg = 100;
  mq_attr.mq_msgsize = MAX_MSG_SIZE;

  mq_attr.mq_flags = 0;


  /* receiver runs at a higher priority than the sender */
  if(taskSpawn("Receiver", 90, 0, 4000, receiver, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR) {
    printf("Receiver task spawn failed\n");
  }
  else
    printf("Receiver task spawned\n");

  if(taskSpawn("Sender", 100, 0, 4000, sender, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR) {
    printf("Sender task spawn failed\n");
  }
  else
    printf("Sender task spawned\n");

   
}
