INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
//CFLAGS= -O3 -msse3 -malign-double -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=

DRIVER=raidtest raid_perftest stripetest

HFILES= raidlib.h
CFILES= raidlib.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${DRIVER}

clean:
	-rm -f *.o *.NEW *~ *Chunk*.bin
	-rm -f ${DRIVER} ${DERIVED} ${GARBAGE}

raidtest:	${OBJS} raidtest.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) raidtest.o $(LIBS)

stripetest:	${OBJS} stripetest.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) stripetest.o $(LIBS)

raid_perftest:	${OBJS} raid_perftest.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) raid_perftest.o $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
