// For the heap message queue, take this message copy version and create a global heap and queue and dequeue pointers to that
// heap instead of the full data in the messages.
//
// Otherwise, the code should not be substantially different.
//
// You can set up an array in the data segement as a global or use malloc and free for a true heap message queue.
//
// Either way, the queue and dequeue should be ZERO copy.
//
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
// #include <mqueue.h> // no longer used
#include <unistd.h>

// On Linux the file systems slash is needed
#define SNDRCV_MQ "/send_receive_hmq"
#define MAX_MSG_SIZE 128
#define ERROR (-1)

#define MAX_COUNT 255
#define SAFE_MESSAGE_SIZE 32
#define NUM_MSGS 1000

// struct mq_attr mq_attr;

pthread_t th_receive, th_send; // create threads
pthread_attr_t attr_receive, attr_send;
struct sched_param param_receive, param_send;

// static char canned_msg[] = "This is a test, and only a test, in the event of real emergency, you would be instructed...."; // Message to be sent

typedef struct ll_node {
  char *message;
  struct ll_node *next;
} ll_node_t;

typedef struct{
  pthread_mutex_t lock;
  ll_node_t *head;
  ll_node_t *tail;
  unsigned char count; //max 255
}heap_t;

heap_t hmsg_queue;
unsigned short msg_idx = 0;
unsigned char running;

/* receives pointer to heap, reads it, and deallocate heap memory */
void *receiver(void *arg)
{
  heap_t *hq = (heap_t *)arg;

  while (1)
  {
    pthread_mutex_lock(&hq->lock);
    if (hq->count == 0)
    {
      pthread_mutex_unlock(&hq->lock);
      if(running == 0) break;
      continue;
    }

    ll_node_t *tmp = hq->head;
    hq->head = tmp->next;
    if (hq->head == NULL)
    {
      hq->tail = NULL;
    }
    hq->count--;
    printf("[CPU %d] RECEIVED message: %s | Count=%d\n", sched_getcpu(), tmp->message, hq->count);
    pthread_mutex_unlock(&hq->lock);

    free(tmp->message);
    tmp->message = NULL;
    free(tmp);
    tmp = NULL;
  }

  return NULL;
}


void *sender(void *arg)
{
  heap_t *hq = (heap_t *)arg;

  while (running)
  {
    //create a new message in heap
    ll_node_t *node = malloc(sizeof(ll_node_t));
    if (node == NULL)
    {
      perror("malloc");
      return NULL;
    }
    node->next = NULL;

    //write message
    node->message = malloc(SAFE_MESSAGE_SIZE);
    if (node->message == NULL)
    {
      perror("malloc");
      return NULL;
    }
    snprintf(node->message, SAFE_MESSAGE_SIZE, "TEST MESSAGE %d", msg_idx);

    pthread_mutex_lock(&hq->lock);
    if (hq->count >= MAX_COUNT)
    {
      pthread_mutex_unlock(&hq->lock);
      free(node->message);
      node->message = NULL;
      free(node);
      node = NULL;
      continue;
    }
    if (hq->tail == NULL)
    {
      hq->head = node;
      hq->tail = node;
    }
    else
    {
      hq->tail->next = node;
      hq->tail = node;
    }
    hq->count++;
    printf("[CPU %d] SENT message: %s | Count=%d\n", sched_getcpu(), node->message, hq->count);
    pthread_mutex_unlock(&hq->lock);

    msg_idx++;
    if (msg_idx > NUM_MSGS)
    {
      running = 0;
    }
  }

  return NULL;
}


int main(void)
{
  int i=0, rc=0;

  int rt_max_prio = sched_get_priority_max(SCHED_FIFO);
  running = 1;

  //set up message queue
  hmsg_queue.head = NULL;
  hmsg_queue.tail = NULL;
  hmsg_queue.count = 0;
  pthread_mutex_init(&hmsg_queue.lock, NULL);
  
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

  struct timespec ts, te;
  double ts_d, te_d;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts_d = (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
  printf("Creating threads at %.5f\n", ts_d);
  
  if((rc=pthread_create(&th_send, &attr_send, sender, &hmsg_queue)) == 0)
  {
    printf("Sender Thread Created\n");
  }
  else 
  {
    perror("Failed to Make Sender Thread\n");
    printf("rc=%d\n", rc);
    return EXIT_FAILURE;
  }

  if((rc=pthread_create(&th_receive, &attr_receive, receiver, &hmsg_queue)) == 0)
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

  clock_gettime(CLOCK_REALTIME, &te);
  te_d = (double)te.tv_sec + (double)te.tv_nsec * 1e-9;
  printf("Joining threads at %.5f\n", te_d);
  printf("Elapsed time = %.5f\n", te_d - ts_d);
  printf("Data rate = %.5f messages per second\n", NUM_MSGS/(te_d - ts_d));

  return EXIT_SUCCESS;
}
