#include "iobuf/iobuf.h"
#include "twoftpd.h"

int respond_code(unsigned code, int final)
{
  return obuf_putu(&outbuf, code) &&
    obuf_putc(&outbuf, final ? ' ' : '-');
}

int respond_end(void)
{
  return obuf_putsflush(&outbuf, "\r\n");
}

int respond(unsigned code, int final, const char* msg)
{
  return respond_code(code, final) &&
    obuf_puts(&outbuf, msg) &&
    respond_end();
}
