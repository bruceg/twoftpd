#include <fcntl.h>
#include <unistd.h>
#include "twoftpd.h"

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
#ifdef LINUX_SENDFILE
  int sent;
  off_t offset;
  struct stat statbuf;
  
  fstat(in, &statbuf);
  offset = 0;
  sent = sendfile(out, in, &offset, statbuf.st_size);
  if (sent != statbuf.st_size) {
    close(out);
    respond(426, 1, "Transfer failed.");
    return 0;
  }
#else
  ssize_t rd;
  ssize_t wr;
  size_t offset;
  char buffer[BUFSIZE];
  
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
#endif
  return 1;
}

int handle_list(void)
{
  int fd;
  int cwd;
  const char** entries;
  struct stat statbuf;
  char buffer[BUFSIZE];
  
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

int handle_nlst(void)
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

int handle_retr(void)
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
