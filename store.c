#include <fcntl.h>
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

static int recvfd(int in, int out)
{
  ssize_t rd;
  ssize_t wr;
  size_t offset;
  char buffer[BUFSIZE];
  
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

int handle_stor(void)
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

int handle_appe(void)
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

int handle_mkd(void)
{
  if (mkdir(req_param, 0777))
    return respond(550, 1, "Could not create directory.");
  return respond(250, 1, "Directory created successfully.");
}

int handle_rmd(void)
{
  if (rmdir(req_param))
    return respond(550, 1, "Could not remove directory.");
  return respond(250, 1, "Directory removed successfully.");
}

int handle_dele(void)
{
  if (unlink(req_param))
    return respond(550, 1, "Could not remove file.");
  return respond(250, 1, "File removed successfully.");
}
