CC = gcc
CFLAGS = -g -Wall

LD = $(CC)
LDFLAGS = -g
LIBS = -lcrypt

PROGS = twoftpd-auth twoftpd-xfer

all: $(PROGS)

twoftpd-auth: auth.o main.o respond.o
	$(LD) $(LDFLAGS) -o $@ auth.o main.o respond.o $(LIBS)

twoftpd-xfer: xfer.o main.o respond.o format.o listdir.o
	$(LD) $(LDFLAGS) -o $@ xfer.o main.o respond.o format.o listdir.o $(LIBS)

auth.o: auth.c twoftpd.h
format.o: format.c twoftpd.h
listdir.o: listdir.c twoftpd.h
main.o: main.c twoftpd.h
respond.o: respond.c twoftpd.h
xfer.o: xfer.c twoftpd.h

clean:
	$(RM) core $(PROGS) *.o
