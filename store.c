/* store.c - twoftpd routines for handling storage-modifying commands
 * Copyright (C) 2001  Bruce Guenter <bruceg@em.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

static char* rnfr_filename = 0;

static int copy_stripcr(ibuf* in, obuf* out)
{
  char buf[iobuf_bufsize];
  char* ptr;
  char* prev;
  unsigned count;
  
  if (ibuf_eof(in)) return 1;
  if (ibuf_error(in) || obuf_error(out)) return 0;
  do {
    if (!ibuf_read_large(in, buf, sizeof buf) && in->count == 0) break;
    count = in->count;
    prev = buf;
    for (;;) {
      if ((ptr = memchr(prev, CR, count)) ==0) break;
      if (!obuf_write(out, prev, ptr - prev)) return 0;
      prev = ptr + 1;
      count = buf + in->count - prev;
    }
    if (!obuf_write(out, prev, count)) return 0;
  } while (!ibuf_eof(in));
  if (!ibuf_eof(in)) return 0;
  if (!obuf_flush(out)) return 0;
  return 1;
}

static int open_copy_close(int flags)
{
  ibuf in;
  obuf out;
  int r;
  
  if (!obuf_open(&out, req_param, flags, 0666, 0))
    return respond(452, 1, "Could not open output file.");
  if (!make_in_connection(&in)) {
    obuf_close(&out);
    return 1;
  }
  r = binary_flag ? iobuf_copyflush(&in, &out) : copy_stripcr(&in, &out);
  ibuf_close(&in);
  obuf_close(&out);
  if (!r)
    return respond(451, 1, "File copy failed.");
  else
    return respond(226, 1, "File received successfully.");
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

int handle_rnfr(void)
{
  struct stat st;
  if (stat(req_param, &st) == -1) {
    if (errno == EEXIST)
      return respond(550, 1, "File does not exist.");
    else
      return respond(450, 1, "Could not locate file.");
  }
  if (rnfr_filename) free(rnfr_filename);
  rnfr_filename = strdup(req_param);
  return respond(350, 1, "OK, file exists.");
}

int handle_rnto(void)
{
  int r;
  if (!rnfr_filename) return respond(425, 1, "Send RNFR first.");
  r = rename(rnfr_filename, req_param);
  free(rnfr_filename);
  rnfr_filename = 0;
  if (r == -1) return respond(550, 1, "Could not rename file.");
  return respond(250, 1, "File renamed.");
}
