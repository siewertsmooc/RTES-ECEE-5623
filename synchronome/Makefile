INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= capturelib.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all: clock_times capture
	mkdir frames

clean:
	-rm -f *.o *.d frames/*.pgm frames/*.ppm
	-rm -f clock_times capture

clock_times: clock_times.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt

capture: capture.o capturelib.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o capturelib.o -lrt

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
