/*******************************************************************

  Unix serial interface test module

  Sam Siewert

********************************************************************/

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h> 
#include <signal.h>

#include "serialutil.h"


#define ERROR -1


/* serialdev speed table */
struct serialdev_setting
{
  char  *speed;
  int   code;
};

static struct serialdev_setting serialdev_speeds[] =
{          /* table of usable baud rates   */

  {    "0",    	B0 },
  {   "50",   	B50 },
  {   "75",   	B75 },
  {  "110",  	B110 },
  {  "300",  	B300 },
  {  "600",  	B600 },
  { "1200", 	B1200 },
  { "2400", 	B2400 },
  { "4800", 	B4800 },
  { "9600", 	B9600 },
#ifdef B14400
  { "14400",    B14400  },
#endif
  { "19200",    B19200  },
  { "38400",    B38400  },
  { "57600",    B57600  },
  { "115200",   B115200 },
  { NULL,   0   }

};


#define EINVAL 1
#define SERIAL_DEV_NEEDS_RET 1

void setdtr (int fd, int on);
void handle_input(int dummy);
void terminal_setup(struct termios *thetty);
void user_control_c(int signum);
void control_c_from_random(int signum);
int have_ctrl_c(char *buf, unsigned length);
void file_tx_setup(void);

static int serial_dev_fd;
static int command_mode = 1;

static int ispeed, ospeed, count,i, offset;
static struct termios serial_port, term, original_term;
static fd_set files;
static char cmd_buff[80];
static FILE *bytelog;
static int hex_input_mode = 0;
static int line_input_mode = 0;
static int file_tx_mode = 0;
static unsigned char hex_input[512];
static unsigned char line_input[512];
static unsigned char file_tx_buff[1024];

static FILE *txfile;
static int file_tx_blk_size;
static int file_tx_num_blks;
static unsigned char file_tx_ack;

#define IPBUFFSIZE 10000

static char ip_buff[IPBUFFSIZE];


int have_ctrl_c(char *buf, unsigned length)
{
  int i;

  for(i=0;i<length;i++)
  {
    if(!(strncmp(&(buf[i]),":\x03", 1)))
    {
      printf("SerialTest: found ctrl-c\n");
      fflush(stdout);
      return 1;
    }

  }

  return 0;

}


int initialize_serial_device(char *port_device_file)
{
   
  serial_dev_fd = open(port_device_file, O_RDWR | O_NOCTTY | O_NDELAY);
   
  if(serial_dev_fd < 0)
  {
    printf("SerialTest: Can't open serial device\n");
    exit(-1);
  }

  else
    printf("SerialTest: Opened serial device %s with fd=%d\n", 
           port_device_file, serial_dev_fd);


  tcgetattr(0, &original_term);
  tcgetattr(0, &term);
   
  term.c_lflag &= ~ICANON;         /*make stdin character at a time*/
  term.c_cc[VMIN] = 1;
  term.c_cc[VTIME] = 0;

#if 0
  if(tcsetattr(0, TCSANOW, &term) != 0)
    printf("SerialTest: Could not change terminal attributes");
#endif

  line_input_mode = 1;
   
  /* set up polling mechanism so that async I/O works */

  FD_ZERO(&files);
  FD_SET(0, &files);
  FD_SET(serial_dev_fd, &files);

  /* Set up the serial device */
 
  tcgetattr(serial_dev_fd, &serial_port);

  for(i = 0; i < NCCS; i++)
       serial_port.c_cc[i] = '\0';        /* no spec chr      */

  serial_port.c_cc[VMIN] = 1;             /* enable one character at a time */
  serial_port.c_cc[VTIME] = 0;

  serial_port.c_iflag = 0;
  serial_port.c_oflag = 0;
  serial_port.c_cflag = 0;
  serial_port.c_lflag = 0;

  serial_port.c_cflag |= CREAD;
  serial_port.c_cflag |= CLOCAL;
  serial_port.c_cflag |= CS7;
  serial_port.c_cflag |= PARENB;

  serialdev_set_raw(&serial_port);
  cfsetospeed(&serial_port, B115200);
  serialdev_set_databits(&serial_port,"8");
  serialdev_set_parity(&serial_port,"N");
  serialdev_set_stopbits(&serial_port,"1");
  printf("SerialTest: setting up serial_port 115200 8,N,1\n");
   

  if(tcsetattr(serial_dev_fd, TCSANOW, &serial_port) != 0)
  {
    printf("SerialTest: Failed to set serial attributes\n");
    exit(-1);
  }
   
  ospeed = cfgetospeed(&serial_port);
  ispeed = cfgetispeed(&serial_port);
  printf("SerialTest: ispeed = %i : ospeed = %i\n", ispeed, ospeed);

  /* DTR ? */
  setdtr(serial_dev_fd, 0);


  /* done with serial dev setup */
  return serial_dev_fd;

} /* end initialize serial device */


void print_menu(void)
{

  if(line_input_mode || hex_input_mode || file_tx_mode)
  {
    printf("**************** Thin System Controller Shell ****************\n");
    printf("<Ctl>-c    to print this menu\n");
    printf("<Ctl>-c, r toggles \\n to \\r translation\n");
    printf("<Ctl>-c, z to configure line discipline\n");
    printf("<Ctl>-c, t to enter tele-type non-canonical entry mode\n");
    printf("<Ctl>-c, c to enter ascii command canonical entry mode\n");
    printf("<Ctl>-c, h to enter hex canonical cmd entry mode\n");
    printf("<Ctl>-c, f to enter simple file tx mode\n");
    printf("<Ctl>-c, e to exit\n");
    printf("**************** Thin System Controller Shell ****************\n");
  }

  else
  {  
    printf("**************** Thin System Controller Shell ****************\n");
    printf("<ESC>-m to print this menu\n");
    printf("<ESC>-r toggles \\n to \\r translation\n");
    printf("<ESC>-z to configure line discipline\n");
    printf("<ESC>-t to enter tele-type non-canonical entry mode\n");
    printf("<ESC>-c to enter ascii command canonical entry mode\n");
    printf("<ESC>-h to enter hex canonical cmd entry mode\n");
    printf("<ESC>-f to enter simple file tx mode\n");
    printf("<ESC>-e to exit\n");
    printf("**************** Thin System Controller Shell ****************\n");
  }

}


void initiate_serial_session(char *session_name, int type)
{
  char c;
  int end_of_session = 0;
  int hexcnt = 0;
  char *line_ptr;
  unsigned char hexval;
  char delim[] = " ";
  int rc;
  char logname[128];
  unsigned char ack_read = 0;
  int len = 0;

  if(type == CMD_LINE_INPUT_MODE)
  {
    line_input_mode = 1;
    file_tx_mode = 0;
    hex_input_mode = 0;
  }

  else if(type == HEX_INPUT_MODE)
  {
    line_input_mode = 0;
    file_tx_mode = 0;
    hex_input_mode = 1;
  }

  else
  {
    line_input_mode = 0;
    file_tx_mode = 0;
    hex_input_mode = 0;
  }
 
  print_menu();

  strcpy(logname, session_name);
  strcat(logname, "_bytelog.out");
 
  if((bytelog = fopen(logname, "w")) == NULL)
  {
    printf("SerialTest: ******** ERROR opening byte log file\n");
    perror("SerialTest:");
  }

  signal(SIGINT, user_control_c);

  /* Initial prompt */

  if(line_input_mode)
  {
    printf("\nCLI dumb terminal mode (remote will provide prompting)\n");
    fflush(stdout);
  }

  else if(hex_input_mode)
  {
    printf("\nhex:");
    fflush(stdout);
  }

  else if(file_tx_mode)
  {
    printf("File transfer mode\n");
    fflush(stdout);
  }
  else
  {
    printf("\n->");
    fflush(stdout);
  }


  while(!end_of_session)
  {

    if(file_tx_mode)
    {

      FD_ZERO(&files);
      FD_SET(serial_dev_fd, &files); 

      while((file_tx_num_blks > 0) && file_tx_mode)
      {

        /* Get a block */

        for(i=0;i<file_tx_blk_size;i++)
        {

          rc = fscanf(txfile, "%2x", (unsigned *)&(file_tx_buff[i]));

          if(rc == EOF)
          {
            printf("\nFile too short!!\n");
            fflush(stdout);
            file_tx_mode = 0;
            break;
          }

        }


        if(file_tx_mode)
        {

          /* Send a block */

          printf("Sending block %d: ", file_tx_num_blks);
          fflush(stdout);
  
          for(i=0;i<file_tx_blk_size;i++)
          {
            write(serial_dev_fd, &(file_tx_buff[i]), 1);
            printf("%2x ", file_tx_buff[i]);
  
          }
  
          printf("\n");
          fflush(stdout);
  
  
          file_tx_num_blks--;
  
          if((select(serial_dev_fd+1, &files, (fd_set*)0, (fd_set*)0, NULL)) != ERROR)
          {

            if(FD_ISSET(serial_dev_fd, &files))
            {
              /* Ack should be just the one ack byte */
              read(serial_dev_fd, &ack_read, 1);

              if(ack_read == file_tx_ack)
              {
                printf("Got block ack\n");
                fflush(stdout);
              }

              else
              {
                printf("**************** BAD block ack, got %2x, expecting %2x\n", ack_read, file_tx_ack);
                fflush(stdout);
              }

              /* Handle input - in case there's extra trash, but shouldn't
                 be the case
               */
              handle_input(0);

            }

          }

          FD_ZERO(&files);
          FD_SET(serial_dev_fd, &files); 

        } /* if file_tx_mode */
        
      } /* while more blocks to send */


      FD_ZERO(&files);
      FD_SET(0, &files);
      FD_SET(serial_dev_fd, &files); 

      file_tx_mode = 0;
      printf("File Sent!!\n\n");
      fflush(stdout);

    }
     
    else if(line_input_mode)
    {     

      if((select(serial_dev_fd+1, &files, (fd_set*)0, (fd_set*)0, NULL)) != ERROR)
      {

        if(FD_ISSET(serial_dev_fd, &files))
        {
          handle_input(0);
        }
  
        if(FD_ISSET(0, &files))
        {
  
          line_ptr = fgets(line_input, (sizeof(line_input)/sizeof(unsigned char)), stdin);

          if(line_ptr != NULL)
          {
    
#if 0
            printf("Read %s\n", line_input);
#endif

	    if(bytelog != NULL)
              fprintf(bytelog, "CLI=%s\n", line_input);
  
            len=strlen(line_input); 
           
            serial_writen(serial_dev_fd, line_input, len); 
#if 0
            printf("Wrote %s to serial\n", line_input);
            fflush(stdout);
#endif

          } /* end if line_ptr != NULL */
#if 0
          printf("CLI:");
          fflush(stdout);
#endif

        } /* end if line input to process */

      } /* end if not select error */

      FD_ZERO(&files);
      FD_SET(0, &files); 
      FD_SET(serial_dev_fd, &files); 
      

    } /* end line_input_mode */


    else if(hex_input_mode)
    {     

      if((select(serial_dev_fd+1, &files, (fd_set*)0, (fd_set*)0, NULL)) != ERROR)
      {

        if(FD_ISSET(serial_dev_fd, &files))
        {
          handle_input(0);
        }
  
        if(FD_ISSET(0, &files))
        {
  
          line_ptr = fgets(hex_input, (sizeof(hex_input)/sizeof(unsigned char)), stdin);
  
          if(line_ptr != NULL)
          {
  
            hexcnt = 0;
  
#if 0
            printf("Read %s\n", hex_input);
#endif

            line_ptr = strtok(hex_input, (const char *)&delim);
  
            while(line_ptr != NULL)
            {
              hexcnt++;
  
#if 0
              printf("hex %d = %s\n", hexcnt, line_ptr);
#endif

              sscanf(line_ptr,"%x", (unsigned *)&hexval);

	      if(bytelog != NULL)
                fprintf(bytelog, "cmd hex=0x%x\n", hexval);

              write(serial_dev_fd, &hexval, 1);
#if 0
              printf("Wrote 0x%x to serial\n", hexval);
              fflush(stdout);
#endif
              line_ptr = strtok(NULL, (const char *)&delim);

            }

          } /* end if line_ptr != NULL */

          printf("hex:");
          fflush(stdout);

        } /* end if hex input to process */

      } /* end if not select error */

      FD_ZERO(&files);
      FD_SET(0, &files); 
      FD_SET(serial_dev_fd, &files); 
      
    } /* end hex_input_mode */

    else /* teletype mode */
    {

      if((select(serial_dev_fd+1, &files, (fd_set*)0, (fd_set*)0, NULL)) != ERROR) 
      {

        if(FD_ISSET(serial_dev_fd, &files))
        {
          handle_input(0);
        }

        if(FD_ISSET(0, &files))
        {

          printf("\n->");
          fflush(stdout);


          if( c=getchar() )
          {

            switch(c)
            {
              case '\n':
                if(SERIAL_DEV_NEEDS_RET && command_mode)
                  write(serial_dev_fd,"\r",1);
              break;
    
              case '\x01B':            /*escape character*/
  
                c=getchar();
  
                if(c == 'm')
                {

                  print_menu();

                  printf("->");
                  fflush(stdout);
                }

                else if(c == 'c')
                {
                  line_input_mode = 1;
                  if(tcsetattr(0, TCSANOW, &original_term) != 0)
                    printf("Could not change terminal attributes");
#if 0
                  printf("\nCLI:");
                  fflush(stdout);
#endif
                }
  
                /* toggle command mode */
                else if(c == 'r')      
                  command_mode = !command_mode;
  
                else if(c == 'f')      /* go into simple file tx mode */
                {
                  file_tx_mode = 1;
                  if(tcsetattr(0, TCSANOW, &original_term) != 0)
                    printf("Could not change terminal attributes");
                  file_tx_setup();
                }

                else if(c == 'h')      /* go into hex mode */
                {
                  hex_input_mode = 1;
                  if(tcsetattr(0, TCSANOW, &original_term) != 0)
                    printf("Could not change terminal attributes");
                  printf("\nhex:");
                  fflush(stdout);
                } 
  
                else if(c == 'z')      /*set up terminal*/
                  terminal_setup(&serial_port);
  
                else if(c == 'e')
                {
                  printf("Exiting serial shell\n");
                  end_of_session = 1;
                }
  
                else
                {
#if 0
                  write(serial_dev_fd, "\x01B", 1);
                  write(serial_dev_fd, &c, 1);
#endif
                  printf("Escape command not valid - ignored\n");
                  fflush(stdout);

                }
  
                break;
  
              default:
  
                write(serial_dev_fd, &c, 1);

		if(bytelog != NULL)
                  fprintf(bytelog, "cmd char=%c\n", c);
  
            } /* end switch */
  
          } /* end if getchar */

          else
            printf("SerialTest: ******** ERROR no input available");

        } /* end if standard input */

      } /* end if not select error */

      FD_ZERO(&files);
      FD_SET(0, &files);
      FD_SET(serial_dev_fd, &files); 

    }  /* end if teletype_mode, i.e. !hex_input_mode and !file_tx_mode*/
	
  } /* end while */
   
  close(serial_dev_fd);

  if(bytelog != NULL)
    fclose(bytelog);

  exit(1);

} /* end initiate serial session */


/*
 * setdtr - control the DTR line on the serial port.
 * This is called from die(), so it shouldn't call die().
 */

void setdtr (int fd, int on)
{
   int serial_port_bits = TIOCM_DTR;
   
   if (on)
   {
	(void) ioctl (fd, TIOCMBIS, &serial_port_bits);
	(void) ioctl (fd, TCFLSH, (caddr_t) TCIFLUSH); /* flush input */
   }

   else
   {
	(void) ioctl (fd, TIOCMBIC, &serial_port_bits);
   }

}


static int countsofar = 0;
static unsigned char inbuff[MAX_INPUT_LENGTH];

void handle_input(int dummy)
{
  int count = 0;
  int i, nleft, nwritten;
 
  for(i=0;i<MAX_INPUT_LENGTH;i++)
    inbuff[i] = '\0';


  if(!line_input_mode)
  {
    printf("\n");
    printf("<-:");
    fflush(stdout);
  }

  do
  {

      count = read(serial_dev_fd, inbuff, (MAX_INPUT_LENGTH-1));
      countsofar += count;
      inbuff[count] = '\0';

      if(hex_input_mode || file_tx_mode)
      {
        for(i=0;i<count;i++)
          printf(" %2x", inbuff[i]);
      }

      else
      {
        nleft=count;
        nwritten=0;

        while(nleft > 0)
        {
          if((nwritten=write(1, &(inbuff[nwritten]), nleft)) <=0)
            break;
      
          nleft -= nwritten;

        }
      }


  } while(count == (MAX_INPUT_LENGTH-1));
  /* on last partial line, do loop exits after processing */ 

  if(line_input_mode)
  {
#if 0
    printf("\nCLI:");
#endif
    fflush(stdout);
  }
  else if(hex_input_mode)
  {
    printf("\nhex:");
    fflush(stdout);
  }
  else if(!file_tx_mode)
  {
    printf("\n->");
    fflush(stdout);
  }

  if(bytelog != NULL)
  {
    fprintf(bytelog, "START REMOTE OUTPUT\n");
    fprintf(bytelog, "prints as=>%s*****", inbuff);
    fprintf(bytelog, "Bytes in hex=");

    for(i=0;i<count;i++)
    {
      fprintf(bytelog, "%x ", inbuff[i]);
      fflush(bytelog);
    }

    fprintf(bytelog, "\nEND REMOTE OUTPUT %d = %d\n", count, countsofar);
  }

}



void file_tx_setup(void)
{
   char instr[256];
   
   printf("Simple File Transfer Setup:");
   
   printf("Filename or full path? ");
   scanf("%s", instr);
   if((txfile=fopen(instr, "r")) == 0)
   {
     perror("Simple file transfer");
     file_tx_mode=0;
     return;
   }

   printf("Block size [bytes]? ");
   scanf("%s", instr);
   sscanf(instr, "%d", (unsigned *)&file_tx_blk_size);
   
   printf("Number of blocks? ");
   scanf("%s", instr);
   sscanf(instr, "%d", (unsigned *)&file_tx_num_blks);

   printf("Ack hex code? ");
   scanf("%s", instr);
   sscanf(instr, "%2x", (unsigned *)&file_tx_ack);
   
   printf("Setup complete\n\n");
   fflush(stdout);
}


void terminal_setup(struct termios *thetty)
{
   char instr[15];
   
   printf("Terminal Setup:");
   
   printf("Speed? ");
   scanf("%s", instr);
   serialdev_set_speed(thetty, instr);
   
   printf("Parity? ");
   scanf("%s", instr);
   serialdev_set_parity(thetty, instr);
   
   printf("Stop Bits? ");
   scanf("%s", instr);
   serialdev_set_stopbits(thetty, instr);
   
   printf("Data Bits? ");
   scanf("%s", instr);
   serialdev_set_databits(thetty, instr);
   
   tcsetattr(serial_dev_fd, TCSANOW, thetty);         /* store new values */
   
   printf("Setup complete\n\n");
   fflush(stdout);
}


/* Supporting functions */

/* Put a terminal line in a transparent state. */
static int
serialdev_set_raw(struct termios *serialdev)
{
  int i;
  int speed;

  for(i = 0; i < NCCS; i++)
        serialdev->c_cc[i] = '\0';        /* no spec chr      */
  serialdev->c_cc[VMIN] = 1;
  serialdev->c_cc[VTIME] = 0;
  serialdev->c_iflag = (IGNBRK | IGNPAR);     /* input flags      */
  serialdev->c_oflag = (0);               /* output flags     */
  serialdev->c_lflag = (0);               /* local flags      */
  speed = (serialdev->c_cflag & CBAUD);       /* save current speed   */
  serialdev->c_cflag = (CREAD|CLOCAL);  /* UART flags       */
  serialdev->c_cflag |= speed;            /* restore speed    */
  return(0);
}


/* Find a serial speed code in the table. */
static int
serialdev_find_speed(char *speed)
{
  int i;

  i = 0;
  while (serialdev_speeds[i].speed != NULL) {
    if (! strcmp(serialdev_speeds[i].speed, speed)) return(serialdev_speeds[i].code);
    i++;
  }
  return(-EINVAL);
}


/* Set the line speed of a terminal line. */
static int
serialdev_set_speed(struct termios *serialdev, char *speed)
{
  int code;

  code = serialdev_find_speed(speed);
  if (code < 0) return(-1);
  serialdev->c_cflag &= ~CBAUD;
  serialdev->c_cflag |= code;

  return(0);
}

/* Set the number of data bits. */
static int
serialdev_set_databits(struct termios *serialdev, char *databits)
{
  serialdev->c_cflag &= ~CSIZE;
  switch(*databits) {
    case '5':
        serialdev->c_cflag |= CS5;
        break;

    case '6':
        serialdev->c_cflag |= CS6;
        break;

    case '7':
        serialdev->c_cflag |= CS7;
        break;

    case '8':
        serialdev->c_cflag |= CS8;
        break;

    default:
        return(-EINVAL);
  }
  return(0);
}

/* Set the number of stop bits. */
static int
serialdev_set_stopbits(struct termios *serialdev, char *stopbits)
{
  switch(*stopbits) {
    case '1':
        serialdev->c_cflag &= ~CSTOPB;
        break;

    case '2':
        serialdev->c_cflag |= CSTOPB;
        break;

    default:
        return(-EINVAL);
  }
  return(0);
}


/* Set the type of parity encoding. */

static int serialdev_set_parity(struct termios *serialdev, char *parity)
{
  switch(toupper(*parity)) {
    case 'N':
        serialdev->c_cflag &= ~(PARENB | PARODD);
        break;  

    case 'O':
        serialdev->c_cflag &= ~(PARENB | PARODD);
        serialdev->c_cflag |= (PARENB | PARODD);
        break;

    case 'E':
        serialdev->c_cflag &= ~(PARENB | PARODD);
        serialdev->c_cflag |= (PARENB);
        break;

    default:
        return(-EINVAL);
  }
  return(0);
}


void user_control_c(int signum)
{
  char ch;

  printf("\n\nSerial session got signal %d, escaping to menu ...\n", signum);
  fflush(stdout);

  print_menu();

  if(tcsetattr(0, TCSANOW, &term) != 0)
  {
    printf("Could not change terminal attributes");
  }

  hex_input_mode = 0;
  line_input_mode = 0;

  printf("\nEnter menu selection:");
  fflush(stdout);
  scanf("%c", &ch);

  if(ch == 'r')
  { 
    /* toggles \\n to \\r translation\n */
    command_mode = !command_mode;
  }

  else if(ch == 'z')
  {
    /* to configure line discipline */ 
    terminal_setup(&serial_port);
  }

  else if(ch == 'h')
  {
    /* to enter hex canonical cmd entry mode */ 
    hex_input_mode = 1;
    if(tcsetattr(0, TCSANOW, &original_term) != 0)
      printf("Could not change terminal attributes");
    printf("\nhex:");
    fflush(stdout);
  }

  else if(ch == 'f')
  {
    /* to enter simple file tx mode */ 
    file_tx_mode = 1;
    if(tcsetattr(0, TCSANOW, &original_term) != 0)
      printf("Could not change terminal attributes");
    file_tx_setup();
  }

  else if(ch == 'c')
  {
    /* to enter canonical command mode */ 
    line_input_mode = 1;
    if(tcsetattr(0, TCSANOW, &original_term) != 0)
      printf("Could not change terminal attributes");

#if 0
    printf("\nCLI:");
    fflush(stdout);
#endif

  }

  else if(ch == 'e')
  {
    close(serial_dev_fd);
    printf("Serial test: Shutting down ...\n");
    fflush(stdout);
    exit(0);
  }

  else /* ch == 't' or anything other than the above */
  {
    /* enter teletype input mode */ 
    line_input_mode = 0;
    file_tx_mode = 0;
    hex_input_mode = 0;
    if(tcsetattr(0, TCSANOW, &term) != 0)
      printf("Could not change terminal attributes");
    printf("\n->");
    fflush(stdout);
  }


  /* reset the signal handler */
  signal(SIGINT, user_control_c);
  return;

}

/* Right out of Steven's */
ssize_t serial_readn(int fd, void *vptr, size_t n)
{
  ssize_t nleft;
  ssize_t nread;
  char *ptr;
 
  ptr = vptr;
  nleft = n;
 
  while(nleft > 0)
  {
    if((nread=read(fd, ptr, nleft)) < 0)
      return nread;
    else if(nread == 0)
      break;
 
    nleft -= nread;
    ptr += nread;
  }
  return (n-nleft);
}
 
/* Right out of Steven's */
ssize_t serial_writen(int fd, const void *vptr, size_t n)
{
  ssize_t nleft;
  ssize_t nwritten;
  const char *ptr;
 
  ptr=vptr;
  nleft=n;
 
  while(nleft > 0)
  {
    if((nwritten=write(fd, ptr, nleft)) <=0)
      return nwritten;
 
    nleft -= nwritten;
    ptr += nwritten;
  }
  return n;
}
