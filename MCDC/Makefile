INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -Wall -O0 -fprofile-arcs -ftest-coverage -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

PRODUCT=sclogic

HFILES=
CFILES= sclogic.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all: sclogic

clean:
	-rm -f *.o *.d *.exe simple *.gcov *.gcno *.gcda

simple: sclogic.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
