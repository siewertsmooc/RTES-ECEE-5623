NCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O3 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread -lrt

PRODUCT=posix_clock posix_linux_demo posix_mq signal_demo

HFILES=
CFILES= posix_clock.c posix_linux_demo.c posix_mq.c signal_demo.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

posix_linux_demo:	posix_linux_demo.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ posix_linux_demo.o $(LIBS)

posix_clock:	posix_clock.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ posix_clock.o $(LIBS)

posix_mq:	posix_mq.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ posix_mq.o $(LIBS)

signal_demo:	signal_demo.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ signal_demo.o $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<
