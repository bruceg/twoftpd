/* socket.c - twoftpd routines for handling sockets
 * Copyright (C) 2001  Bruce Guenter <bruceg@em.ca>
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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"
#include "unix/nonblock.h"

/* State variables */
static int socket_fd = -1;
static struct sockaddr_in socket_addr;
static struct sockaddr_in remote_addr;
static enum { NONE, PASV, PORT } connect_mode = NONE;

static int accept_connection(void)
{
  int fd;
  int size;
  fd_set fds;
  struct timeval to;
  
  size = sizeof remote_addr;
  FD_ZERO(&fds);
  FD_SET(socket_fd, &fds);
  to = timeout;
  if (select(socket_fd+1, &fds, 0, 0, &to) == 0) {
    respond(425, 1, "Timed out waiting for the connection.");
    return -1;
  }
  if ((fd = accept(socket_fd, (struct sockaddr*)&remote_addr, &size)) == -1)
    respond(425, 1, "Could not accept the connection.");
  else {
    close(socket_fd);
    socket_fd = -1;
    connect_mode = NONE;
  }
  return fd;
}

static int start_connection(void)
{
  int fd;
  struct pollfd p;
  
  if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) {
    respond(425, 1, "Could not allocate a socket.");
    return -1;
  }
  if (!nonblock_on(fd)) {
    close(fd);
    respond(425, 1, "Could not set flags on socket.");
    return -1;
  }
  connect(fd, (struct sockaddr*)&remote_addr, sizeof remote_addr);
  p.fd = fd;
  p.events = POLLOUT;
  if (poll(&p, 1, timeout.tv_usec/1000 + timeout.tv_sec*1000) != 1 ||
      p.revents != POLLOUT) {
    close(fd);
    respond(425, 1, "Could not build the connection.");
    return -1;
  }
  if (!nonblock_off(fd)) {
    close(fd);
    respond(425, 1, "Could not reset flags on socket.");
    return -1;
  }
  return fd;
}

static int make_connection_fd(void)
{
  if (connect_mode == NONE) {
    respond(425, 1, "No PORT or PASV commands have been issued.");
  }
  else {
    respond(150, 1, "Opening data connection.");
    return (connect_mode == PASV) ? accept_connection() : start_connection();
  }
  return -1;
}

int make_in_connection(ibuf* in)
{
  int fd;
  if ((fd = make_connection_fd()) == -1) return 0;
  if (!ibuf_init(in, fd, 1, 0)) return 0;
  return 1;
}

int make_out_connection(obuf* out)
{
  int fd;
  if ((fd = make_connection_fd()) == -1) return 0;
  if (!obuf_init(out, fd, 1, 0)) return 0;
  return 1;
}

static unsigned char strtoc(const char* str, char** end)
{
  long tmp;
  tmp = strtol(str, end, 10);
  if (tmp < 0 || tmp > 0xff) *end = 0;
  return tmp;
}

static int parse_addr(const char* str)
{
  char* end;
  unsigned long host;
  unsigned short port;
  
  host = strtoc(str, &end) << 24; if (*end != ',') return 0;
  host |= strtoc(end+1, &end) << 16; if (*end != ',') return 0;
  host |= strtoc(end+1, &end) << 8; if (*end != ',') return 0;
  host |= strtoc(end+1, &end); if (*end != ',') return 0;
  port = strtoc(end+1, &end) << 8; if (*end != ',') return 0;
  port |= strtoc(end+1, &end); if (*end != 0) return 0;
  memset(&remote_addr, 0, sizeof remote_addr);
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_addr.s_addr = htonl(host);
  remote_addr.sin_port = htons(port);
  connect_mode = PORT;
  return 1;
}

static int make_socket(void)
{
  int size;
  
  size = sizeof *&socket_addr;
  if (socket_fd != -1) close(socket_fd);
  if ((socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) != -1) {
    memset(&socket_addr, 0, sizeof socket_addr);
    socket_addr.sin_family = AF_INET;
    inet_aton(tcplocalip, &socket_addr.sin_addr);
    if (!bind(socket_fd, (struct sockaddr*)&socket_addr, sizeof socket_addr) &&
	!listen(socket_fd, 1) &&
	!getsockname(socket_fd, (struct sockaddr*)&socket_addr, &size)) {
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
  char buffer[6*4+1];
  unsigned long addr;
  unsigned short port;
  if (!make_socket()) return respond(550, 1, "Could not create socket.");
  addr = ntohl(socket_addr.sin_addr.s_addr);
  port = ntohs(socket_addr.sin_port);
  snprintf(buffer, sizeof buffer, "=%lu,%lu,%lu,%lu,%u,%u",
	   (addr>>24)&0xff, (addr>>16)&0xff, (addr>>8)&0xff, addr&0xff,
	   (port>>8)&0xff, port&0xff);
  return respond(227, 1, buffer);
}

int handle_port(void)
{
  if (!parse_addr(req_param))
    return respond(501, 1, "Can't parse your PORT address.");
  return respond(200, 1, "PORT OK.");
}
