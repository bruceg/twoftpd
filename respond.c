#include <stdio.h>
#include <string.h>
#include "twoftpd.h"

int respond(unsigned code, int final, const char* msg)
{
  printf("%03d%c%s\r\n", code, final ? ' ' : '-', msg);
  return !fflush(stdout);
}
