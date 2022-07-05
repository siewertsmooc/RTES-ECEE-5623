NCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O3 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread -lrt

PRODUCT=posix_mq

HFILES=
CFILES= posix_mq.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

posix_mq:	posix_mq.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ posix_mq.o $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<
