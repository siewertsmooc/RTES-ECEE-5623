INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc
AS=as

CDEFS=
CFLAGS= -O0 $(INCLUDE_DIRS) $(CDEFS)
ASFLAGS= $(INCLUDE_DIRS)
LIBS= 

HFILES= lcmwrapper.h

CFILES= lcmwrapper.c lcmc.c
ASMFILES= lcmarm.s


#all:	lcmc.gen.asm lcmwrapper_c lcmwrapper_asm
all:	lcmc.gen.asm lcmwrapper_c

clean:
	-rm -f *.o *.asm
	-rm -f lcmwrapper_c lcmwrapper_asm

distclean:
	-rm -f *.o *.d


# Uncomment lcmwrapper that uses assembly code or the version that uses the C code
# as needed to test linking the ASM or the C code of the same name

#lcmwrapper_asm: lcmintel.o lcmasmwrapper.o
#	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ lcmintel.o lcmasmwrapper.o

lcmwrapper_asm: lcmarm.o lcmasmwrapper.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ lcmarm.o lcmasmwrapper.o

lcmwrapper_c: lcmc.o lcmwrapper.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ lcmc.o lcmwrapper.o -lrt


# C code generation of object code
#
lcmc.o: lcmc.c
	$(CC) $(CFLAGS) -c $<

lcmwrapper.o: lcmwrapper.c
	$(CC) $(CFLAGS) -c $<

lcmasmwrapper.o: lcmasmwrapper.c
	$(CC) $(CFLAGS) -c $<


# Assembly code generation by C compiler and creation of object code from 
# hand written assembly lcmarm.s
#
# If the hand written lcmarm.s does not work on your architecture, e.g., you are on x86, then
# you can use the generated ASM lcm.gen.asm to replace lcmarm.s instead.
#
# As an example, I have saved off some Intel architecture generated code as "lcmintel.s".
#
lcmc.gen.asm: lcmc.c
	$(CC) -O0 -S $< -o $@

lcmarm.o: lcmarm.s
	$(AS) $(ASFLAGS) $< -o $@

lcmintel.o: lcmintel.s
	$(AS) $(ASFLAGS) $< -o $@

