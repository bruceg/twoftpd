#include <fcntl.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

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
