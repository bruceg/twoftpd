#include <string.h>
#include <unistd.h>
#include "twoftpd.h"

int respond(unsigned code, int final, const char* msg)
{
  static char buffer[4 + BUFSIZE + 2];
  static char* const bufptr = buffer + 4;
  size_t msglen;

  buffer[0] = ((code / 100) % 10) + '0';
  buffer[1] = ((code / 10) % 10) + '0';
  buffer[2] = (code % 10) + '0';
  buffer[3] = final ? ' ' : '-';

  msglen = msg ? strlen(msg) : 0;
  if (msglen >= BUFSIZE)
    msglen = BUFSIZE;
  memcpy(bufptr, msg, msglen);
  bufptr[msglen++] = CR;
  bufptr[msglen++] = LF;

  return write(1, buffer, 4 + msglen) == 4 + msglen;
}
