INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= seqgenex0.c seqgen.c seqgen2.c seqgen3.c seqv4l2.c capturelib.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	seqgenex0 seqgen seqgen2 seqgen3 seqv4l2 clock_times capture

clean:
	-rm -f *.o *.d frames/*.pgm frames/*.ppm
	-rm -f seqgenex0 seqgen seqgen2 seqgen3 seqv4l2 clock_times capture

seqgenex0: seqgenex0.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt

seqv4l2: seqv4l2.o capturelib.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o capturelib.o -lpthread -lrt

seqgen3: seqgen3.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt

seqgen2: seqgen2.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt

seqgen: seqgen.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt

clock_times: clock_times.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt

capture: capture.o capturelib.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o capturelib.o -lrt

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
