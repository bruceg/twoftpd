PACKAGE = twoftpd
VERSION = 0.2

install_prefix =
prefix = /usr/local
bindir = $(prefix)/bin
mandir = $(prefix)/man
man1dir = $(mandir)/man1

install = install
installbin = $(install) -m 555
installdir = $(install) -d
installsrc = $(install) -m 444

PROGS = twoftpd-auth twoftpd-xfer
SCRIPTS = 
MAN1S = 
SERVICES =
DOCS = COPYING NEWS README
DIST = $(DOCS) $(MAN1S) Makefile 

CC = gcc
CFLAGS = -g -Wall -DBINDIR=\"$(bindir)\"

LD = $(CC)
LDFLAGS = -g
LIBS = -lcrypt

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

install: all
	$(installdir) $(install_prefix)$(bindir)
	$(installbin) $(PROGS) $(SCRIPTS) $(install_prefix)$(bindir)

clean:
	$(RM) core $(PROGS) *.o
