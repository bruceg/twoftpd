#include "iobuf/iobuf.h"
#include "twoftpd.h"

int respond(unsigned code, int final, const char* msg)
{
  return obuf_putu(&outbuf, code) &&
    obuf_putc(&outbuf, final ? ' ' : '-') &&
    obuf_puts(&outbuf, msg) &&
    obuf_putsflush(&outbuf, "\r\n");
}
