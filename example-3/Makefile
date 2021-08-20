INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O2 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread

PRODUCT=testdigest

HFILES= md5.h config.h sha1.h crc.h sha2.h
CFILES= testdigest.c md5.c sha1.c crc.c sha2.c

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
