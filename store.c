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
#include "backend.h"

static int open_copy_close(int flags)
{
  ibuf in;
  obuf out;

  if (!obuf_open(&out, req_param, flags, 0666, 0))
    return respond(452, 1, "Could not open output file.");
  if (!make_in_connection(&in)) {
    obuf_close(&out);
    return 1;
  }
  if (!iobuf_copyflush(&in, &out)) {
    ibuf_close(&in);
    obuf_close(&out);
    return respond(451, 1, "File copy failed.");
  }
  else {
    ibuf_close(&in);
    obuf_close(&out);
    return respond(226, 1, "File received successfully.");
  }
}

int handle_stor(void)
{
  return open_copy_close(OBUF_CREATE | OBUF_TRUNCATE);
}

int handle_appe(void)
{
  return open_copy_close(OBUF_APPEND);
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

static char* rnfr_filename = 0;

int handle_rnfr(void)
{
  struct stat st;
  if (stat(req_param, &st) == -1)
    if (errno == EEXIST)
      return respond(550, "File does not exist.");
    else
      return respond(450, "Could not locate file.");
  if (rnfr_filename) free(rnfr_filename);
  rnfr_filename = strdup(req_param);
  return respond(350, "OK, file exists.");
}

int handle_rnto(void)
{
  int r;
  if (!rnfr_filename) return respond(425, "Send RNFR first.");
  r = rename(rnfr_filename, req_param);
  free(rnfr_filename);
  rnfr_filename = 0;
  if (r == -1) return respond(550, "Could not rename file.");
  return respond(250, "File renamed.");
}
