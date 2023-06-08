INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g -Wall $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= syslogtest.c syslogrt.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	syslogtest syslogrt

clean:
	-rm -f *.o *.d
	-rm -f syslogtest syslogrt

syslogrt: syslogrt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread

syslogtest: syslogtest.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
