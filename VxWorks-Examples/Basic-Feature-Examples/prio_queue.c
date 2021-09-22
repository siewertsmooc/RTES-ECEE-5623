/* Function: Priority queue demonstration
Sam Siewert - 7/23/97 
*/

#include "stdio.h"
#include "stdlib.h"
#include "semLib.h"
#include "msgQLib.h"
#include "errnoLib.h"

MSG_Q_ID ServerToClient;

int Server_tid, Client_tid;

void Server(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10)
{
  int i;

  while(1) {
    for(i=0;i<10;i++) {
      if(msgQSend(ServerToClient, "situation NORMAL", 16, WAIT_FOREVER, MSG_PRI_NORMAL) == ERROR) printf("Server: Error sending client message\n");
    }
    if(msgQSend(ServerToClient, "situation CRITICAL", 16, WAIT_FOREVER, MSG_PRI_URGENT) == ERROR) printf("Server: Error sending client message\n");

    taskDelay(100);
  }

}


#define MSG_LENGTH 80

void Client(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10)
{
  char buffer[MSG_LENGTH];
  int byteCount;

  while(1) {
    byteCount = msgQReceive(ServerToClient, buffer, MSG_LENGTH, WAIT_FOREVER);
    buffer[byteCount] = '\0';
    printf("Client: got message \"%s\" from server\n", buffer);
    taskDelay(100);
  }

}

#define MAX_MSGS 100

void prio_queue()
{

  if ((ServerToClient = msgQCreate(MAX_MSGS, MSG_LENGTH, MSG_Q_PRIORITY)) == NULL) printf("error creating ServerToClient queue");

  if ((Client_tid = taskSpawn("client", 75, 0, (1024*8), Client,0,0,0,0,0,0, 0,0,0,0)) == ERROR) printf("error spawinig client");
  if ((Server_tid = taskSpawn("server", 75, 0, (1024*8), Server,0,0,0,0,0,0, 0,0,0,0)) == ERROR) printf("error spawinig server");

  taskDelay(3000);

  taskDelete(Client_tid);
  taskDelete(Server_tid);
  msgQDelete(ServerToClient);

}
