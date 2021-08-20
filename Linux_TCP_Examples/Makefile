INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=

PRODUCT=inet_client inet_server

HFILES=
CFILES= inet_client.c inet_server.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

inet_client:	inet_client.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ inet_client.o $(LIBS)

inet_server:	inet_server.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ inet_server.o $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
