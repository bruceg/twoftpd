/* twoftpd-bind-port.c -- twoftpd port (20) binding hack
 * Copyright (C) 2002  Bruce Guenter <bruceg@em.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <net/socket.h>
#include <str/str.h>
#include <sysdeps.h>

#include <msg/msg.h>

#include "twoftpd.h"

const char program[] = "twoftpd-bind-port";
const int msg_show_pid = 1;

static ipv4port port;
static ipv4addr ip;
static pid_t ppid;

unsigned timeout = 1;

void die(const char* msg)
{
  respond(421, 1, msg);
  exit(111);
}

void mainloop(int sock)
{
  char code;
  int newsock;
  iopoll_fd fds;
  fds.fd = sock;
  fds.events = IOPOLL_READ;
  for (;;) {
    switch (iopoll_restart(&fds, 1, timeout*1000)) {
    case -1:
      return;
    case 0:
      /* Timed out */
      if (kill(ppid, 0) == -1) return;
      continue;
    default:
      if (read(sock, &code, 1) != 1) return;
      code = 0;
      if ((newsock = socket_tcp()) == -1) {
	error1sys("Creating socket failed");
	code = 1;
      }
      else if (!socket_reuse(newsock)) {
	error1sys("Setting flags on socket failed");
	code = 2;
      }
      else if (!socket_bind4(newsock, &ip, port)) {
	error1sys("Binding socket failed");
	code = 3;
      }
      if (write(sock, &code, 1) != 1)
	error1sys("Sending the result code failed");
      if (!code)
	if (!socket_sendfd(sock, newsock))
	  error1sys("Sending the bound socket failed");
      close(newsock);
    }
  }
}

int main(int argc, char* argv[])
{
  static str fdstr;
  const char* tmp;
  const char* end;
  int sock[2];
  
  if (argc < 2) {
    respond(421, 1, "Configuration error, insufficient paramenters.");
    return 0;
  }

  if ((tmp = getenv("TCPLOCALIP")) == 0 ||
      !ipv4_parse(tmp, &ip, &end) ||
      *end != 0) {
    respond(421, 1, "Configuration error, $TCPLOCALIP is not set or is invalid.");
    return 0;
  }
  
  if ((tmp = getenv("TWOFTPD_BIND_PORT")) != 0) {
    port = strtoul(tmp, (char**)&end, 10);
    if (*end != 0) port = 20;
  }
  else port = 20;

  if (!socket_pairdgm(sock)) {
    respond_syserr(421, "Could not create socket pair.");
    return 0;
  }

  if (!str_catu(&fdstr, sock[0]) ||
      setenv("TWOFTPD_BIND_PORT_FD", fdstr.s, 1) == -1) {
    respond_syserr(421, "Could not set $TWOFTPD_BIND_PORT_FD.");
    return 0;
  }

  ppid = getpid();
  switch (fork()) {
  case -1:
    respond_syserr(421, "Could not fork.");
    break;
  case 0:
    close(sock[0]);
    mainloop(sock[1]);
    break;
  default:
    close(sock[1]);
    execvp(argv[1], argv+1);
    respond_syserr(421, "Could not execute command line.");
  }
  return 0;
}
