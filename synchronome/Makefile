INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -Wall -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= capturelib.c sequencer.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all: capture sequencer

clean:
	-rm -f *.o *.d frames/*.pgm frames/*.ppm
	-rm -f capture

capture: capture.o capturelib.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o capturelib.o -lrt

sequencer: sequencer.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread

.c.o:
	$(CC) $(CFLAGS) -c $<
