INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O3 $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= sumdigits.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	sumdigits

clean:
	-rm -f *.o *.d
	-rm -f sumdigits

distclean:
	-rm -f *.o *.d
	-rm -f sumdigits

sumdigits: sumdigits.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
