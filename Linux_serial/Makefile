#*******************************************************************************
#
# Sam Siewert
#
# Makefile for Unix-based serial test tool
#
# Tested on Linux  2.4.2-2 #1 Sun Apr 8 20:41:30 EDT 2001 i686 unknown
#
#*******************************************************************************

OBJECTS =	serialutil.o \
		serial_test.o

CC = gcc

INCLUDES = -I.

LDFLAGS =

LIBS = -lm

all: serial_test inet_client inet_server

inet_clent: inet_clent.o
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@
 
inet_server: inet_server.o
	$(CC) $(LDFLAGS) $(LIBS) $^ -o $@

serial_test: serial_test.o serialutil.o
	$(CC) $(LDFLAGS) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean :
	echo *.o *.i *.s *~ \#*# core .#* .new* serial_test inet_server inet_client
	rm -f *.o *.i *.s *~ \#*# core .#* .new* serial_test inet_server inet_client
