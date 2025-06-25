ImNCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O -g $(INCLUDE_DIRS) $(CDEFS) -DLINUX
LIBS=-lpthread -lrt

HFILES=

CFILES1= pthread3ok.c 
CFILES2= deadlock.c
CFILES3= pthread3.c
CFILES4= deadlock_timeout.c
CFILES5= pthread3amp.c

SRCS1= ${HFILES} ${CFILES1}
SRCS2= ${HFILES} ${CFILES2}
SRCS3= ${HFILES} ${CFILES3}
SRCS4= ${HFILES} ${CFILES4}
SRCS5= ${HFILES} ${CFILES5}

OBJS1= ${CFILES1:.c=.o}
OBJS2= ${CFILES2:.c=.o}
OBJS3= ${CFILES3:.c=.o}
OBJS4= ${CFILES4:.c=.o}
OBJS5= ${CFILES5:.c=.o}

all: pthread3 pthread3ok pthread3amp deadlock deadlock_timeout

clean:
	-rm -f *.o *.d *.exe pthread3ok pthread3 pthread3amp deadlock deadlock_timeout

pthread3: pthread3.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS3) $(LIBS)

pthread3ok: pthread3ok.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)

pthread3amp: pthread3amp.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS5) $(LIBS)

deadlock: deadlock.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS2) $(LIBS)

deadlock_timeout: deadlock_timeout.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS4) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<
