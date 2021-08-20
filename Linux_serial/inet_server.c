#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#define NSTRS 3
#define DEFAULT_PORT 54321

char *test_strs[NSTRS] = {
	"This is the first server string.\n",
	"This is the second server string.\n",
	"This is the third server string.\n"
};

extern int errno;
extern void int_handler();
extern void broken_pipe_handler();
extern void serve_clients();

static int server_sock, client_sock;
static int fromlen, i, j, num_sets;
static char c;
static FILE *fp;
static struct sockaddr_in server_sockaddr, client_sockaddr;

main()
{
  char hostname[64];
  struct hostent *hp;
  struct linger opt;
  int sockarg;

  gethostname(hostname, sizeof(hostname));

  if((hp = (struct hostent*) gethostbyname(hostname)) == NULL)
  {
    fprintf(stderr, "Error: %s host unknown.\n", hostname);
    exit(-1);
  }

  if((server_sock=socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Server: socket");
    exit(-1);
  }

  bzero((char*) &server_sockaddr, sizeof(server_sockaddr));
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(DEFAULT_PORT);
  bcopy (hp->h_addr, &server_sockaddr.sin_addr, hp->h_length);

  /* Bind address to the socket */
  if(bind(server_sock, (struct sockaddr *) &server_sockaddr,
     sizeof(server_sockaddr)) < 0) 
  {
    perror("Server: bind");
    exit(-1);
  }

  /* turn on zero linger time so that undelivered data is discarded when
     socket is closed
   */
  opt.l_onoff = 1;
  opt.l_linger = 0;

  sockarg = 1;
 
  setsockopt(server_sock, SOL_SOCKET, SO_LINGER, (char*) &opt, sizeof(opt));
  setsockopt(client_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&sockarg, sizeof(int));
  signal(SIGINT, int_handler);
  signal(SIGPIPE, broken_pipe_handler);

  serve_clients();

}


/* Listen and accept loop function */
void serve_clients()
{

  for(;;)
  {

    /* Listen on the socket */
    if(listen(server_sock, 5) < 0)
    {
      perror("Server: listen");
      exit(-1);
    }

    /* Accept connections */
    if((client_sock=accept(server_sock, 
                           (struct sockaddr *)&client_sockaddr,
                           &fromlen)) < 0) 
    {
      perror("Server: accept");
      exit(-1);
    }

    fp = fdopen(client_sock, "r");

    recv(client_sock, (char *)&num_sets, sizeof(int), 0);
    printf("number of sets = %d\n", num_sets);

    for(j=0;j<num_sets;j++)
    {

      /* Send strings to the client */
      for (i=0; i<NSTRS; i++)
        send(client_sock, test_strs[i], strlen(test_strs[i]), 0);

      /* Read client strings and print them out */
      for (i=0; i<NSTRS; i++)
      {
        while((c = fgetc(fp)) != EOF)
        {
	  if(num_sets < 4)
            putchar(c);

          if(c=='\n')
            break;
        } /* end while */

      } /* end for NSTRS */

    } /* end for num_sets */

    close(client_sock);

  } /* end for ever */

}


/* Close sockets after a Ctrl-C interrupt */

void int_handler()
{
  char ch;

  printf("Enter y to close sockets or n to keep open:");
  scanf("%c", &ch);

  if(ch == 'y')
  {
    printf("\nsockets are being closed\n");
    close(client_sock);
    close(server_sock);
  }

  printf("Server: Shutting down ...\n");

  exit(0);

}


void broken_pipe_handler()
{
  char ch;

  printf("Enter y to continue serving clients or n to halt:");
  scanf("%c", &ch);

  if(ch == 'y')
  {
    printf("\nwill continue serving clients\n");
    serve_clients();
  }

  else
  {
    printf("Server: Shutting down ...\n");
    exit(0);
  }

}

