PACKAGE = twoftpd
VERSION = 0.5

install_prefix =
prefix = /usr/local
bindir = $(prefix)/bin
mandir = $(prefix)/man
man1dir = $(mandir)/man1

install = install
installbin = $(install) -m 555
installdir = $(install) -d
installsrc = $(install) -m 444

PROGS = twoftpd-anon twoftpd-auth twoftpd-xfer
SCRIPTS = 
MAN1S = 
SERVICES = twoftpd
DOCS = COPYING NEWS README
DIST = $(DOCS) $(MAN1S) Makefile 

CC = gcc
CFLAGS = -g -Wall

LD = $(CC)
LDFLAGS = -g
LIBS = -lcrypt

all: $(PROGS)

twoftpd-anon: backend.o anon.o main.o respond.o list.o listdir.o \
	nlst.o retr.o socket.o stat.o state.o
	$(LD) $(LDFLAGS) -o $@ backend.o anon.o main.o respond.o \
	list.o listdir.o nlst.o retr.o socket.o stat.o state.o $(LIBS)

twoftpd-auth: frontend.o main.o respond.o auth.o
	$(LD) $(LDFLAGS) -o $@ frontend.o main.o respond.o auth.o $(LIBS)

twoftpd-xfer: backend.o xfer.o main.o respond.o list.o listdir.o \
	nlst.o retr.o socket.o stat.o state.o store.o
	$(LD) $(LDFLAGS) -o $@ backend.o xfer.o main.o respond.o \
	list.o listdir.o nlst.o retr.o socket.o stat.o state.o store.o $(LIBS)

anon.o: anon.c twoftpd.h
auth.o: auth.c twoftpd.h
backend.o: backend.c twoftpd.h
format.o: format.c twoftpd.h
frontend.o: frontend.c twoftpd.h
list.o: list.c twoftpd.h
listdir.o: listdir.c twoftpd.h
main.o: main.c twoftpd.h
nlst.o: nlst.c twoftpd.h
respond.o: respond.c twoftpd.h
retr.o: retr.c twoftpd.h
socket.o: socket.c twoftpd.h
stat.o: stat.c twoftpd.h
state.o: state.c twoftpd.h
store.o: store.c twoftpd.h
xfer.o: xfer.c twoftpd.h

install: all
	$(installdir) $(install_prefix)$(bindir)
	$(installbin) $(PROGS) $(SCRIPTS) $(install_prefix)$(bindir)

clean:
	$(RM) core $(PROGS) *.o
