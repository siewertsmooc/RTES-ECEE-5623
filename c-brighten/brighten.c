#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

void readppm(unsigned char *buffer, int *bufferlen, 
             char *header, int *headerlen,
             unsigned *rows, unsigned *cols, unsigned *chans,
             char *file)
{
    char *aline=NULL;  size_t linelen; FILE *filep;
    char magic[2]; unsigned col, row, sat, channels=3;
    int nread=0, toread=0, fd=0;
    *headerlen=0;

    filep=fopen(file, "r");

    // read and validate header
    if((*headerlen += getline(&aline, &linelen, filep)) < 0)
        {perror("getline"); exit(-1);}
    strcat(header, aline);
    sscanf(aline, "%s", magic);
    if(strncmp(magic, "P6", 2) == 0) channels=3; else channels=1;

    // ignore comment line or print for debug
    if((*headerlen += getline(&aline, &linelen, filep)) < 0)
        {perror("getline"); exit(-1);}
    strcat(header, aline);

    if((*headerlen += getline(&aline, &linelen, filep)) < 0)
        {perror("getline"); exit(-1);}
    sscanf(aline, "%u %u", &col, &row);
    strcat(header, aline);
    *bufferlen=row*col*channels;  toread=*bufferlen;
    *rows=row, *cols=col, *chans=channels;

    if((*headerlen += getline(&aline, &linelen, filep)) < 0)
        {perror("getline"); exit(-1);}
    sscanf(aline, "%u", &sat);
    strcat(header, aline);

    printf("%s", header);

    printf("read %d bytes, buffer=%p, toread=%d\n", nread, buffer, toread);

    do
    {
        if((nread=fread(buffer, 1, (col*row*channels), filep)) == 0)
        { 
            if(feof(filep))
                { printf("completed readppm\n"); break; }
            else
                { perror("readppm"); exit(-1); }
        }

        buffer+=nread;
        toread=toread-nread;
        printf("read %d bytes, buffer=%p, toread=%d\n", nread, buffer, toread);
    } while(toread > 0 && (!feof(filep)));

    printf("readppm done\n\n");
    close(filep);
}


void writeppm(unsigned char *buffer, int bufferlen,
              char *header, int headerlen,
              char *file)
{
    FILE *filep;
    int nwritten=0, towrite=0;

    filep=fopen(file, "w");

    printf("wrote %d bytes, header=%p, towrite=%d\n", nwritten, header, towrite);

    do
    {
        if((nwritten=fwrite(header, 1, headerlen, filep)) == 0)
        { 
            if(feof(filep))
                { printf("completed writeppm header\n"); break; }
            else
                { perror("writeppm header"); exit(-1); }
        }

        header+=nwritten;
        towrite=towrite-nwritten;
        printf("wrote %d bytes, header=%p, towrite=%d\n", nwritten, header, towrite);
    } while(towrite > 0);
    towrite=0; nwritten=0;

    printf("wrote %d bytes, buffer=%p, towrite=%d\n", nwritten, buffer, towrite);
    do
    {
        if((nwritten=fwrite(buffer, 1, bufferlen, filep)) == 0)
        { 
            if(feof(filep))
                { printf("completed writeppm\n"); break; }
            else
                { perror("writeppm"); exit(-1); }
        }

        buffer+=nwritten;
        towrite=towrite-nwritten;
        printf("wrote %d bytes, buffer=%p, towrite=%d\n", nwritten, buffer, towrite);
    } while(towrite > 0);

    close(filep);
}


#define PIXIDX ((i*col*chan)+(j*chan)+k)
#define SAT (255)

void main(int argc, char *argv[])
{
  char header[512];
  unsigned char img[640*480*3], newimg[640*480*3];
  int bufflen, hdrlen; unsigned row=0, col=0, chan=0, pix; int i, j, k;
  double alpha=1.25;  unsigned char beta=25;

  header[0]='\0';
  readppm(img, &bufflen, header, &hdrlen, &row, &col, &chan, argv[1]);

  for(i=0; i < row; i++)
    for(j=0; j < col; j++)
      for(k=0; k < chan; k++)
      {
          newimg[PIXIDX] = (pix=(unsigned)((img[PIXIDX])*alpha)+beta) > SAT ? SAT : pix;
      }

  writeppm(newimg, bufflen, header, hdrlen, "brighter.ppm");
}
