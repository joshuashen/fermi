CC=			gcc
CFLAGS=		-g -Wall -O2
DFLAGS=		#-DNDEBUG
OBJS=		utils.o seq.o sais.o saux.o rld.o index.o exact.o
PROG=		fmg
INCLUDES=	
LIBS=		-lm -lz

.SUFFIXES:.c .o

.c.o:
		$(CC) -c $(CFLAGS) $(DFLAGS) $(INCLUDES) $< -o $@

all:$(PROG)

fmg:$(OBJS) main.o
		$(CC) $(CFLAGS) $(DFLAGS) $(OBJS) main.o -o $@ $(LIBS)

rld.o:rld.h
index.o:rld.h
exact.o:exact.h rld.h kstring.h

clean:
		rm -fr gmon.out *.o a.out $(PROG) *~ *.a *.dSYM
