INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g -Wcpp $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lrt

HFILES= 
CFILES= capture.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	capture

clean:
	-rm -f *.o *.d capture
	-rm -f frames/*

distclean:
	-rm -f *.o *.d

capture: ${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
