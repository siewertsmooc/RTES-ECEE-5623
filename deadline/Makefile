NCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread -lrt

PRODUCT=deadline

HFILES=
CFILES= deadline.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

deadline:	deadline.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ deadline.o $(LIBS)

depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
