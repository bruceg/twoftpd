/* socket.c - twoftpd routines for handling sockets
 * Copyright (C) 2008  Bruce Guenter <bruce@untroubled.org>
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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fmt/multi.h>
#include <net/socket.h>
#include "twoftpd.h"
#include "backend.h"
#include <sysdeps.h>
#include <unix/nonblock.h>

/* State variables */
static int socket_fd = -1;
static ipv4addr socket_ip;
static unsigned short socket_port;
static ipv4addr remote_ip;
static unsigned short remote_port;
static ipv4addr client_ip;
static ipv4addr server_ip;
static enum { NONE, PASV, PORT } connect_mode = NONE;

static int respond_timedoutconn(void)
{
  return respond(425, 1, "Timed out waiting for connection.");
}

static int respond_connfailed(void)
{
  return respond_syserr(425, "Connection failed");
}

static int respond_connaborted(void)
{
  return respond(425, 1, "Connection aborted by incoming command.");
}

static int accept_connection(void)
{
  int fd;
  iopoll_fd pf[2];
  
  pf[0].fd = 0;
  pf[0].events = IOPOLL_READ;
  pf[0].revents = 0;
  pf[1].fd = socket_fd;
  pf[1].events = IOPOLL_READ;
  switch (iopoll_restart(pf, 2, timeout*1000)) {
  case 2: break;
  case 1: break;
  case 0: respond_timedoutconn(); return -1;
  default: respond_connfailed(); return -1;
  }
  if (pf[0].revents) {
    respond_connaborted();
    return -1;
  }
  if ((fd = socket_accept4(socket_fd, &remote_ip, &remote_port)) == -1) {
    respond_connfailed();
    return -1;
  }
  close(socket_fd);
  socket_fd = -1;
  connect_mode = NONE;
  if (!nonblock_on(fd)) {
    respond_syserr(425, "Could not set flags on socket");
    close(fd);
    return -1;
  }
  return fd;
}

static int make_connect_socket(void)
{
  int fd;
  char buf;

  fd = -1;
  if (bind_port_fd != -1) {
    if (write(bind_port_fd, &buf, 1) == 1 &&
	read(bind_port_fd, &buf, 1) == 1 &&
	buf == 0)
      fd = socket_recvfd(bind_port_fd);
  }
  if (fd == -1) {
    if ((fd = socket_tcp()) == -1) {
      respond_syserr(425, "Could not allocate a socket");
      return -1;
    }
    if (!socket_reuse(fd) ||
	!socket_bind4(fd, &server_ip, 0)) {
      respond_syserr(425, "Could not set flags on socket");
      close(fd);
      return -1;
    }
  }
  return fd;
}

static int start_connection(void)
{
  int fd;
  iopoll_fd pf[2];
  
  if ((fd = make_connect_socket()) == -1) return -1;
  if (!nonblock_on(fd)) {
    respond_syserr(425, "Could not set flags on socket");
    close(fd);
    return -1;
  }

  if (socket_connect4(fd, &remote_ip, remote_port)) return fd;

  if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
    respond_connfailed();
    return -1;
  }

  pf[0].fd = 0;
  pf[0].events = IOPOLL_READ;
  pf[0].revents = 0;
  pf[1].fd = fd;
  pf[1].events = IOPOLL_WRITE;
  switch (iopoll_restart(pf, 2, timeout*1000)) {
  case 0:
    respond_timedoutconn();
    break;
  case 2:
  case 1:
    if (pf[0].revents) {
      respond_connaborted();
      break;
    }
    if (socket_connected(fd)) return fd;
    /* No break, fall through */
  default:
    respond_connfailed();
  }
  close(fd);
  return -1;
}

static int make_connection_fd(void)
{
  int fd;
  
  if (connect_mode == NONE) {
    fd = -1;
    respond(425, 1, "No PORT or PASV commands have been issued.");
  }
  else {
    fd = (connect_mode == PASV) ? accept_connection() : start_connection();
    if (fd != -1) respond(150, 1, "Opened data connection.");
  }
  respond_start_xfer();
  return fd;
}

int make_in_connection(ibuf* in)
{
  int fd;
  if ((fd = make_connection_fd()) == -1) return 0;
  if (!ibuf_init(in, fd, 0, IOBUF_NEEDSCLOSE, 0)) return 0;
  in->io.timeout = timeout * 1000;
  return 1;
}

int make_out_connection(obuf* out)
{
  int fd;
  if ((fd = make_connection_fd()) == -1) return 0;
  socket_cork(fd);
  if (!socket_linger(fd, 1, timeout)) {
    respond_syserr(425, "Could not set flags on socket");
    close(fd);
    return 0;
  }
  if (!obuf_init(out, fd, 0, IOBUF_NEEDSCLOSE, 0)) return 0;
  out->io.timeout = timeout * 1000;
  return 1;
}

int close_out_connection(obuf* out)
{
  if (!obuf_flush(out)) {
    obuf_close(out);
    return 0;
  }
  socket_uncork(out->io.fd);
  return obuf_close(out);
}

static unsigned char strtoc(const char* s, const char** end)
{
  long tmp;
  tmp = strtol(s, (char**)end, 10);
  if (tmp < 0 || tmp > 0xff) *end = s;
  return tmp;
}

static int scan_ip(const char* s, char sep,
		   ipv4addr* addr, const char** endptr)
{
  const char* end;
  addr->addr[0] = strtoc(s, &end); if (*end != sep) return 0;
  addr->addr[1] = strtoc(end+1, &end); if (*end != sep) return 0;
  addr->addr[2] = strtoc(end+1, &end); if (*end != sep) return 0;
  addr->addr[3] = strtoc(end+1, &end);
  *endptr = end;
  return 1;
}

int parse_localip(const char* s)
{
  const char* end;
  return scan_ip(s, '.', &server_ip, &end) && *end == 0;
}

int parse_remoteip(const char* s)
{
  const char* end;
  return scan_ip(s, '.', &client_ip, &end) && *end == 0;
}
  
static int parse_addr(const char* s)
{
  const char* end;
  if (!scan_ip(s, ',', &remote_ip, &end) || *end != ',') return 0;
  remote_port = strtoc(end+1, &end) << 8; if (*end != ',') return 0;
  remote_port |= strtoc(end+1, &end); if (*end != 0) return 0;
  connect_mode = PORT;
  return 1;
}

static int make_accept_socket(void)
{
  if (socket_fd != -1) close(socket_fd);
  if ((socket_fd = socket_tcp()) != -1) {
    if (socket_bind4(socket_fd, &server_ip, 0) &&
	socket_listen(socket_fd, 1) &&
	socket_getaddr4(socket_fd, &socket_ip, &socket_port)) {
      connect_mode = PASV;
      return 1;
    }
    else {
      close(socket_fd);
      socket_fd = -1;
    }
  }
  connect_mode = NONE;
  return 0;
}

int handle_pasv(void)
{
  char buffer[6*4+25];
  if (!make_accept_socket())
    return respond_syserr(425, "Could not create socket");
  buffer[fmt_multi(buffer, "{Entering Passive Mode (}u{,}u{,}u{,}u{,}u{,}u{).",
		   socket_ip.addr[0], socket_ip.addr[1],
		   socket_ip.addr[2], socket_ip.addr[3],
		   (socket_port>>8)&0xff, socket_port&0xff)] = 0;
  return respond(227, 1, buffer);
}

int handle_port(void)
{
  if (!parse_addr(req_param))
    return respond(501, 1, "Can't parse your PORT address.");
  if (memcmp(&remote_ip, &client_ip, sizeof client_ip))
    return respond(501, 1, "PORT IP does not match client address.");
  return respond_ok();
}
