INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS= -DPMU_ANALYSIS
CFLAGS= -O3 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

PRODUCT=perfmon

HFILES=
CFILES= perfmon.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

${PRODUCT}:	${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
