INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O3 $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= perflib.h
CFILES= perfmon.c perflib.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	perfmon pthread

clean:
	-rm -f *.o *.d
	-rm -f perfmon pthread

distclean:
	-rm -f *.o *.d
	-rm -f pthread

pthread: pthread.o perflib.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o perflib.o -lpthread

perfmon: ${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o perflib.o $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
