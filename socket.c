#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "twoftpd.h"

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
  if ((fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) return -1;
  if (connect(fd, (struct sockaddr*)&remote_addr, sizeof remote_addr)) {
    respond(425, 1, "Could not build the connection.");
    close(fd);
    return -1;
  }
  return fd;
}

int make_connection(void)
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

static void format_addr(char* buffer)
{
  unsigned long addr;
  unsigned short port;
  addr = ntohl(socket_addr.sin_addr.s_addr);
  port = ntohs(socket_addr.sin_port);
  sprintf(buffer, "=%lu,%lu,%lu,%lu,%u,%u",
	  (addr>>24)&0xff, (addr>>16)&0xff, (addr>>8)&0xff, addr&0xff,
	  (port>>8)&0xff, port&0xff);
}

static int make_socket(void)
{
  int size;
  
  size = sizeof *&socket_addr;
  if (socket_fd != -1) close(socket_fd);
  if ((socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) != -1) {
    memset(&socket_addr, 0, sizeof socket_addr);
    socket_addr.sin_family = AF_INET;
    inet_aton(getenv("TCPLOCALIP"), &socket_addr.sin_addr);
    if (!bind(socket_fd, (struct sockaddr*)&socket_addr, sizeof socket_addr) &&
	!listen(socket_fd, 1) &&
	!getsockname(socket_fd, (struct sockaddr*)&socket_addr, &size)) {
      connect_mode = PORT;
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
  char buffer[BUFSIZE];
  if (!make_socket()) return respond(550, 1, "Could not create socket.");
  format_addr(buffer);
  return respond(227, 1, buffer);
}

int handle_port(void)
{
  if (!parse_addr(req_param))
    return respond(501, 1, "Can't parse your PORT address.");
  return respond(200, 1, "PORT OK.");
}
