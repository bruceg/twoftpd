#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "twoftpd.h"

int do_chroot;

/* Temporary buffers */
static char buffer[BUFSIZE];
static struct stat statbuf;

/* State variables */
static int binary_flag = 0;
static int socket_fd = -1;
static struct sockaddr_in socket_addr;
static struct sockaddr_in remote_addr;
static enum { NONE, PASV, PORT } connect_mode = NONE;

/*****************************************************************************/
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

static int make_connection(void)
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

static int sendfd(int in, int out)
{
  ssize_t rd;
  ssize_t wr;
  size_t offset;
  
  for (;;) {
    rd = read(in, buffer, sizeof buffer);
    if (rd == -1) {
      close(out);
      respond(451, 1, "Read failed.");
      return 0;
    }
    if (rd == 0) break;
    for (offset = 0; rd; offset += wr, rd -= wr) {
      wr = write(out, buffer + offset, rd);
      if (wr == -1) {
	close(out);
	respond(426, 1, "Write failed.");
	return 0;
      }
    }
  }
  return 1;
}

static int recvfd(int in, int out)
{
  ssize_t rd;
  ssize_t wr;
  size_t offset;
  
  for (;;) {
    rd = read(in, buffer, sizeof buffer);
    if (rd == -1) {
      close(out);
      respond(426, 1, "Read from socket failed.");
      return 0;
    }
    if (rd == 0) break;
    for (offset = 0; rd; offset += wr, rd -= wr) {
      wr = write(out, buffer + offset, rd);
      if (wr == -1) {
	close(out);
	respond(451, 1, "Write failed.");
	return 0;
      }
    }
  }
  return 1;
}

static unsigned char strtoc(const char* str, char** end)
{
  long tmp;
  tmp = strtol(str, end, 10);
  if (tmp < 0 || tmp > 0xff) *end = 0;
  return tmp;
}

static int parse_addr(const char* str, struct sockaddr_in* addr)
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
  memset(addr, 0, sizeof *addr);
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = htonl(host);
  addr->sin_port = htons(port);
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
    inet_aton(getenv("TCPLOCALIP"), &socket_addr.sin_addr);
    if (!bind(socket_fd, (struct sockaddr*)&socket_addr, sizeof socket_addr) &&
	!listen(socket_fd, 1) &&
	!getsockname(socket_fd, (struct sockaddr*)&socket_addr, &size))
      return 1;
    else {
      close(socket_fd);
      socket_fd = -1;
    }
  }
  return 0;
}

/*****************************************************************************/
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

static int handle_size(void)
{
  if (stat(req_param, &statbuf) == -1)
    return respond(550, 1, "Could not determine file size.");
  sprintf(buffer, "%lu", statbuf.st_size);
  return respond(213, 1, buffer);
}

static int handle_mdtm(void)
{
  if (stat(req_param, &statbuf) == -1)
    return respond(550, 1, "Could not determine file time.");
  strftime(buffer, sizeof buffer, "%Y%m%d%H%M%S",
	   gmtime(&statbuf.st_mtime));
  return respond(213, 1, buffer);
}

static int handle_pasv(void)
{
  unsigned long addr;
  unsigned short port;
  
  if (!make_socket()) return respond(550, 1, "Could not create socket.");
  connect_mode = PASV;
  addr = ntohl(socket_addr.sin_addr.s_addr);
  port = ntohs(socket_addr.sin_port);
  sprintf(buffer, "=%lu,%lu,%lu,%lu,%u,%u",
	  (addr>>24)&0xff, (addr>>16)&0xff, (addr>>8)&0xff, addr&0xff,
	  (port>>8)&0xff, port&0xff);
  return respond(227, 1, buffer);
}

static int handle_port(void)
{
  if (!parse_addr(req_param, &remote_addr))
    return respond(501, 1, "Can't parse your PORT address.");
  connect_mode = PORT;
  return respond(200, 1, "PORT OK.");
}

static int handle_list(void)
{
  int fd;
  int cwd;
  const char** entries;

  if (req_param) {
    if ((cwd = pushd(req_param)) == -1)
      return respond(550, 1, "Could not list directory.");
  }
  else
    cwd = -1;
  if ((entries = listdir(".")) == 0) {
    fchdir(cwd);
    return respond(550, 1, "Could not list directory.");
  }

  if ((fd = make_connection()) == -1) return 1;

  while (*entries) {
    if (stat(*entries, &statbuf) != -1) {
      format_stat(&statbuf, *entries, buffer);
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
  const char** entries;

  if ((entries = listdir(req_param ? req_param : ".")) == 0)
    return respond(550, 1, "Could not list directory.");

  if ((fd = make_connection()) == -1) return 1;

  while (*entries) {
    write(fd, *entries, strlen(*entries));
    write(fd, "\r\n", 2);
    ++entries;
  }
  close(fd);
  return respond(226, 1, "Transfer complete.");
}

static int handle_retr(void)
{
  int in;
  int out;
  if ((in = open(req_param, O_RDONLY)) == -1)
    return respond(550, 1, "Could not open input file.");
  if ((out = make_connection()) == -1) return 1;
  if (!sendfd(in, out)) return 1;
  close(out);
  return respond(226, 1, "File sent successfully.");
}

static int handle_stor(void)
{
  int in;
  int out;
  if ((out = open(req_param, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
    return respond(452, 1, "Could not open output file.");
  if ((in = make_connection()) == -1) return 1;
  if (!recvfd(in, out)) return 1;
  close(in);
  close(out);
  return respond(226, 1, "File received successfully.");
}

static int handle_appe(void)
{
  int in;
  int out;
  if ((out = open(req_param, O_WRONLY | O_APPEND, 0666)) == -1)
    return respond(452, 1, "Could not open output file.");
  if ((in = make_connection()) == -1) return 1;
  if (!recvfd(in, out)) return 1;
  close(in);
  close(out);
  return respond(226, 1, "File received successfully.");
}

static int handle_mkd(void)
{
  if (mkdir(req_param, 0777))
    return respond(550, 1, "Could not create directory.");
  return respond(250, 1, "Directory created successfully.");
}

static int handle_rmd(void)
{
  if (rmdir(req_param))
    return respond(550, 1, "Could not remove directory.");
  return respond(250, 1, "Directory removed successfully.");
}

static int handle_dele(void)
{
  if (unlink(req_param))
    return respond(550, 1, "Could not remove file.");
  return respond(250, 1, "File removed successfully.");
}

verb verbs[] = {
  { "PASS", 0,           handle_pass },
  { "ACCT", 0,           handle_pass },
  { "TYPE", 0,           handle_type },
  { "STRU", 0,           handle_stru },
  { "MODE", 0,           handle_mode },
  { "CWD",  0,           handle_cwd },
  { "PWD",  handle_pwd,  0 },
  { "CDUP", handle_cdup, 0 },
  { "PASV", handle_pasv, 0 },
  { "PORT", 0,           handle_port },
  { "LIST", handle_list, handle_list },
  { "NLST", handle_nlst, handle_nlst },
  { "SIZE", 0,           handle_size },
  { "MDTM", 0,           handle_mdtm },
  { "RETR", 0,           handle_retr },
  { "STOR", 0,           handle_stor },
  { "APPE", 0,           handle_appe },
  { "MKD",  0,           handle_mkd  },
  { "RMD",  0,           handle_rmd  },
  { "DELE", 0,           handle_dele },
  /* Compatibility verbs as defined by RFC1123 */
  { "XCWD", 0,           handle_cwd },
  { "XPWD", handle_pwd,  0 },
  { "XCUP", handle_cdup, 0 },
  { "XMKD", 0,           handle_mkd  },
  { "XRMD", 0,           handle_rmd  },
  { 0, 0, 0 }
};

static int load_tables(void)
{
  /* Load some initial values that cannot be loaded once the chroot is done. */
  
  /* A call to localtime should load and cache the timezone setting. */
  now = time(0);
  localtime(&now);
  return 1;
}

#define FAIL(MSG) do{ respond(421, 1, MSG); return 0; }while(0)

int startup(int argc, char* argv[])
{
  uid_t uid;
  gid_t gid;
  char* home;
  char* tmp;

  do_chroot = !getenv("NOCHROOT");
  
  if ((tmp = getenv("UID")) == 0) FAIL("Missing $UID.");
  if ((uid = atoi(tmp)) <= 0) FAIL("Invalid $UID.");
  if ((tmp = getenv("GID")) == 0) FAIL("Missing $GID.");
  if ((gid = atoi(tmp)) <= 0) FAIL("Invalid $GID.");
  if ((home = getenv("HOME")) == 0) FAIL("Missing $HOME.");
  if (chdir(home)) FAIL("Could not chdir to $HOME.");
  if (!load_tables()) FAIL("Loading startup tables failed.");
  if (do_chroot) if (chroot(".")) FAIL("Could not chroot.");
  if (setgid(gid)) FAIL("Could not set GID.");
  if (setuid(uid)) FAIL("Could not set UID.");
  return respond(202, 1, "Ready to transfer files.");
}
