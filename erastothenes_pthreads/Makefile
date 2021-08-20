INCLUDE_DIRS = 
LIB_DIRS = 
CC = gcc

CDEFS=
CFLAGS= -O3 $(INCLUDE_DIRS) $(CDEFS)
#CFLAGS= -O3 -msse3 -malign-double $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread

#PRODUCT=erastpt
PRODUCT=erast

HFILES= 
#CFILES= erastpt.c
CFILES= erast.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~
	-rm -f ${PRODUCT} ${DERIVED} ${GARBAGE}

${PRODUCT}:	${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
