#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"
#include "hassendfile.h"

#ifdef HASLINUXSENDFILE
#include <sys/sendfile.h>
#endif

static int retr_bin(ibuf* in, obuf* out)
{
#ifdef HASLINUXSENDFILE
  int sent;
  off_t offset;
  struct stat statbuf;
  
  fstat(in->io.fd, &statbuf);
  offset = 0;
  sent = sendfile(out->io.fd, in->io.fd, &offset, statbuf.st_size);
  if (sent != statbuf.st_size) {
    respond(426, 1, "Transfer failed.");
    return 0;
  }
#else
  if (!iobuf_copyflush(in, out)) {
    respond(426, 1, "Write failed.");
    return 0;
  }
#endif
  return 1;
}

static int retr_asc(ibuf* in, obuf* out)
{
  char buf[iobuf_bufsize];
  char* ptr;
  char* prev;
  unsigned count;
  
  if (ibuf_eof(in)) return 1;
  if (ibuf_error(in) || obuf_error(out)) return 0;
  do {
    if (!ibuf_read(in, buf, sizeof buf) && in->count == 0) break;
    count = in->count;
    prev = buf;
    ptr = memchr(prev, LF, count);
    while (ptr) {
      if (!obuf_write(out, prev, ptr - prev)) return 0;
      if (!obuf_write(out, "\r\n", 2)) return 0;
      prev = ptr + 1;
      count = buf + in->count - prev;
      ptr = memchr(prev, LF, count);
    }
    if (!obuf_write(out, prev, count)) return 0;
  } while (!ibuf_eof(in));
  return ibuf_eof(in);
}
      
int handle_retr(void)
{
  ibuf in;
  obuf out;
  int result;
  
  if (!ibuf_open(&in, req_param, 0))
    return respond(550, 1, "Could not open input file.");
  if (!make_out_connection(&out)) {
    ibuf_close(&in);
    return 1;
  }
  result = binary_flag ? retr_bin(&in, &out) : retr_asc(&in, &out);
  ibuf_close(&in);
  obuf_close(&out);
  return result ? respond(226, 1, "File sent successfully.") : 1;
}
