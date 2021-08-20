#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>


#define NSTRS 3
#define MAX_IT 1
#define LOCAL_PORT 54321

char *strs[NSTRS] = {
	"This is the first client string.\n",
	"This is the second client string.\n",
	"This is the third client string.\n"
};

extern int errno;
extern void broken_pipe_handler();

main(argc, argv)
int argc;
char **argv;
{
  char c;
  FILE *fp;
  int i, j, client_sock, len, num_sets;
  struct hostent *hp;
  struct sockaddr_in client_sockaddr;
  struct linger opt;
  int sockarg;
 

  if(argc < 2)
  {
    printf("Usage: inet_client <server hostname>\n");
    exit(-1);
  }

  if((hp = gethostbyname(argv[1])) == NULL)
  {
    fprintf(stderr, "Error: %s unknown host.\n", argv[1]);
    exit(-1);
  }

  if((client_sock=socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("client: socket");
    exit(-1);
  }

  client_sockaddr.sin_family = AF_INET;
  client_sockaddr.sin_port = htons(LOCAL_PORT);
  bcopy(hp->h_addr, &client_sockaddr.sin_addr, hp->h_length);

  /* discard undelivered data on closed socket */ 
  opt.l_onoff = 1;
  opt.l_linger = 0;

  sockarg = 1;

  setsockopt(client_sock, SOL_SOCKET, SO_LINGER, (char*) &opt, sizeof(opt));

  setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&sockarg, sizeof(int));

  if(connect(client_sock, (struct sockaddr*)&client_sockaddr,
     sizeof(client_sockaddr)) < 0) 
  {
    perror("client: connect");
    exit(-1);
  }

  signal(SIGPIPE, broken_pipe_handler);

  fp = fdopen(client_sock, "r");

  num_sets = MAX_IT;

  send(client_sock, (char *)&num_sets, sizeof(int), 0);

  for(j=0;j<num_sets;j++)
  {

    /* Read server strings and print them out */
    for (i=0; i<NSTRS; i++)
    {
      while((c = fgetc(fp)) != EOF)
      {
        putchar(c);

        if(c=='\n')
        break;

      }
    }

    /* Send strings to the server */
    for (i=0; i<NSTRS; i++)
      send(client_sock, strs[i], strlen(strs[i]), 0);

  }

  close(client_sock);

  exit(0);

}


void broken_pipe_handler()
{
  printf("\nbroken pipe signal received\n");
}
