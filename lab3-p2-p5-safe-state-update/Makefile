NCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O3 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread -lrt

PRODUCT=update_state

HFILES=
CFILES= update_state.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

update_state:	update_state.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ update_state.o -lm $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<
