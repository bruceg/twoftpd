#include <stdio.h>
#include <time.h>
#include "twoftpd.h"
#include "backend.h"

int handle_size(void)
{
  struct stat statbuf;
  char buffer[BUFSIZE];
  if (stat(req_param, &statbuf) == -1)
    return respond(550, 1, "Could not determine file size.");
  sprintf(buffer, "%lu", statbuf.st_size);
  return respond(213, 1, buffer);
}

int handle_mdtm(void)
{
  struct stat statbuf;
  char buffer[BUFSIZE];
  if (stat(req_param, &statbuf) == -1)
    return respond(550, 1, "Could not determine file time.");
  strftime(buffer, sizeof buffer, "%Y%m%d%H%M%S",
	   gmtime(&statbuf.st_mtime));
  return respond(213, 1, buffer);
}
