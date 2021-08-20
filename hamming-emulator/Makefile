INCLUDE_DIRS = 
LIB_DIRS = 

CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
//CFLAGS= -O3 -Wall -pg -msse3 -malign-double -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=

DRIVER=ecctest

HFILES= ecclib.h
CFILES= ecctest.c ecclib.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${DRIVER}

clean:
	-rm -f *.o *.NEW *~ gmon.out
	-rm -f ${DRIVER} ${DERIVED} ${GARBAGE}

${DRIVER}:	${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
