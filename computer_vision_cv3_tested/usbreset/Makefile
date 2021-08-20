INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

PRODUCT=usbreset

HFILES=
CFILES= usbreset.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all: usbreset

clean:
	-rm -f *.o *.d *.exe usbreset

usbreset: usbreset.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
