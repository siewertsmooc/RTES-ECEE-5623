INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= seqgenex0.c seqgen.c seqgen2.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	seqgenex0 seqgen seqgen2 clock_times

clean:
	-rm -f *.o *.d
	-rm -f seqgenex0 seqgen seqgen2 clock_times

seqgenex0: seqgenex0.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt

seqgen2: seqgen2.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt

seqgen: seqgen.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt

clock_times: clock_times.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -lrt

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
