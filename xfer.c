#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <netinet/in.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "twoftpd.h"

static int binary_flag = 0;
static char buffer[BUFSIZE];

static int socket_fd = -1;
static struct sockaddr_in socket_addr;
static struct sockaddr_in remote_addr;

static int handle_pass(void)
{
  return respond(202, 1, "Access has already been granted.");
}

static int handle_type(void)
{
  if (!strcasecmp(req_param, "A") || !strcasecmp(req_param, "A N")) {
    binary_flag = 0;
    return respond(200, 1, "Transfer mode changed to ASCII.");
  }
  if (!strcasecmp(req_param, "I") || !strcasecmp(req_param, "L 8")) {
    binary_flag = 1;
    return respond(200, 1, "Transfer mode changed to BINARY.");
  }
  return respond(501, 1, "Unknown transfer type.");
}

static int handle_stru(void)
{
  if (!strcasecmp(req_param, "F"))
    return respond(200, 1, "OK.");
  return respond(504, 1, "Invalid parameter.");
}

static int handle_mode(void)
{
  if (!strcasecmp(req_param, "S"))
    return respond(200, 1, "OK.");
  return respond(504, 1, "Invalid parameter.");
}

static int handle_cwd(void)
{
  if (chdir(req_param))
    return respond(550, 1, "Directory change failed.");
  return respond(250, 1, "Changed directory.");
}

static int handle_pwd(void)
{
  size_t len;
  if (!getcwd(buffer+1, sizeof buffer-2))
    return respond(550, 1, "Could not determine current working directory.");
  buffer[0] = '"';
  len = strlen(buffer);
  buffer[len++] = '"';
  buffer[len] = 0;
  return respond(257, 1, buffer);
}

static int handle_cdup(void)
{
  if (chdir(".."))
    return respond(550, 1, "Directory change failed.");
  return respond(257, 1, "Changed directory.");
}

static int handle_pasv(void)
{
  unsigned long addr;
  unsigned short port;
  int size;
  
  if (socket_fd != -1) close(socket_fd);
  socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
  if (socket_fd == -1) return respond(550, 1, "Could not create socket.");
  memset(&socket_addr, 0, sizeof socket_addr);
  socket_addr.sin_family = AF_INET;
  inet_aton(getenv("TCPLOCALIP"), &socket_addr.sin_addr);
  if (bind(socket_fd, (struct sockaddr*)&socket_addr, sizeof socket_addr)) {
    close(socket_fd);
    return respond(550, 1, "Could not set up socket.");
  }
  if (listen(socket_fd, 1)) {
    close(socket_fd);
    socket_fd = -1;
    return respond(550, 1, "Could not listen to socket.");
  }
  size = sizeof socket_addr;
  if (getsockname(socket_fd, (struct sockaddr*)&socket_addr, &size)) {
    close(socket_fd);
    socket_fd = -1;
    return respond(550, 1, "Could not determine local port number.");
  }
  addr = ntohl(socket_addr.sin_addr.s_addr);
  port = ntohs(socket_addr.sin_port);
  sprintf(buffer, "=%lu,%lu,%lu,%lu,%u,%u",
	  (addr>>24)&0xff, (addr>>16)&0xff, (addr>>8)&0xff, addr&0xff,
	  (port>>8)&0xff, port&0xff);
  return respond(227, 1, buffer);
}

static int pushd(const char* path)
{
  int cwd;
  if ((cwd = open(".", O_RDONLY)) == -1) return -1;
  if (chdir(path) == -1) {
    close(cwd);
    return -1;
  }
  return cwd;
}

static int handle_list(void)
{
  int fd;
  int size;
  int cwd;
  const char** entries;
  struct stat sb;

  cwd = -1;
  entries = 0;
  if (req_param) {
    if ((cwd = pushd(req_param)) != -1)
      entries = listdir(".");
  }
  else
    entries = listdir(".");
  if (!entries) {
    fchdir(cwd);
    return respond(550, 1, "Could not list directory.");
  }

  size = sizeof remote_addr;
  fd = accept(socket_fd, (struct sockaddr*)&remote_addr, &size);
  close(socket_fd);
  socket_fd = -1;
  if (fd == -1) return respond(550, 1, "Could not accept the connection.");
  if (!respond(150, 1, "Opening ASCII mode data connection.")) return 0;

  while (*entries) {
    if (stat(*entries, &sb) != -1) {
      format_stat(&sb, *entries, buffer);
      write(fd, buffer, strlen(buffer));
    }
    ++entries;
  }
  close(fd);
  fchdir(cwd);
  return respond(226, 1, "Transfer complete.");
}

static int handle_nlst(void)
{
  int fd;
  int size;
  int cwd;
  const char** entries;

  cwd = -1;
  entries = 0;
  if (req_param) {
    if ((cwd = pushd(req_param)) != -1)
      entries = listdir(".");
  }
  else
    entries = listdir(".");
  if (!entries) {
    fchdir(cwd);
    return respond(550, 1, "Could not list directory.");
  }

  size = sizeof remote_addr;
  fd = accept(socket_fd, (struct sockaddr*)&remote_addr, &size);
  close(socket_fd);
  socket_fd = -1;
  if (fd == -1) return respond(550, 1, "Could not accept the connection.");
  if (!respond(150, 1, "Opening ASCII mode data connection.")) return 0;

  while (*entries) {
    write(fd, *entries, strlen(*entries));
    write(fd, "\r\n", 2);
    ++entries;
  }
  close(fd);
  fchdir(cwd);
  return respond(226, 1, "Transfer complete.");
}

verb verbs[] = {
  { "PASS", 0,           handle_pass },
  { "ACCT", 0,           handle_pass },
  { "TYPE", 0,           handle_type },
  { "STRU", 0,           handle_stru },
  { "MODE", 0,           handle_mode },
  { "CWD",  0,           handle_cwd },
  { "XCWD", 0,           handle_cwd },
  { "PWD",  handle_pwd,  0 },
  { "XPWD", handle_pwd,  0 },
  { "CDUP", handle_cdup, 0 },
  { "XCUP", handle_cdup, 0 },
  { "PASV", handle_pasv, 0 },
  { "LIST", handle_list, handle_list },
  { "NLST", handle_nlst, handle_nlst },
  { 0, 0, 0 }
};

#define FAIL(MSG) do{ respond(421, 1, MSG); return 0; }while(0)

int startup(int argc, char* argv[])
{
  uid_t uid;
  gid_t gid;
  char* home;
  char* tmp;
  
  if ((tmp = getenv("UID")) == 0) FAIL("Missing $UID.");
  if ((uid = atoi(tmp)) <= 0) FAIL("Invalid $UID.");
  if ((tmp = getenv("GID")) == 0) FAIL("Missing $GID.");
  if ((gid = atoi(tmp)) <= 0) FAIL("Invalid $GID.");
  if ((home = getenv("HOME")) == 0) FAIL("Missing $HOME.");
  if (chdir(home)) FAIL("Could not chdir to $HOME.");
  /* if (chroot(".")) FAIL("Could not chroot."); */
  if (setgid(gid)) FAIL("Could not set GID.");
  if (setuid(uid)) FAIL("Could not set UID.");
  return respond(202, 1, "Ready to transfer files.");
}
