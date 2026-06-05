INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -std=c11 $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= pthread.c pthread_fifo.c pthread_sem.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	pthread pthread_fifo pthread_sem

clean:
	-rm -f *.o *.d
	-rm -f perfmon pthread pthread_fifo pthread_sem

distclean:
	-rm -f *.o *.d
	-rm -f pthread pthread_fifo pthread_sem

pthread: pthread.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread

pthread_fifo: pthread_fifo.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread

pthread_sem: pthread_sem.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
