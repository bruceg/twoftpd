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

static unsigned long network_bytes;
static str rnfr_filename;

static int copy(ibuf* in, obuf* out)
{
  char buf[iobuf_bufsize];
  char* ptr;
  char* prev;
  unsigned count;
  
  if (obuf_error(out)) return 0;
  network_bytes = 0;
  for (;;) {
    if (!ibuf_read(in, buf, sizeof buf) && in->count == 0) {
      if (ibuf_eof(in)) break;
      return 0;
    }
    count = in->count;
    network_bytes += count;
    prev = buf;
    if (!binary_flag) {
      while (count) {
	if ((ptr = memchr(prev, CR, count)) ==0) break;
	if (!obuf_write(out, prev, ptr - prev)) return 0;
	count -= ptr + 1 - prev;
	prev = ptr + 1;
      }
    }
    if (!obuf_write(out, prev, count)) return 0;
  }
  if (!obuf_flush(out)) return 0;
  return 1;
}

static int open_copy_close(int flags)
{
  int r;
  ibuf in;
  obuf out;
  
  if (!open_out(&out, req_param, flags))
    return respond(452, 1, "Could not open output file.");
  if (!make_in_connection(&in)) {
    obuf_close(&out);
    return 1;
  }
  r = copy(&in, &out);
  ibuf_close(&in);
  obuf_close(&out);
  if (r)
    return respond_bytes(226, "File received successfully", network_bytes, 0);
  else
    return respond_bytes(451, "File store failed", network_bytes, 0);
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
  if (!qualify_validate(req_param)) return 1;
  if (mkdir(fullpath.s+1, 0777))
    return respond(550, 1, "Could not create directory.");
  return respond(257, 1, "Directory created successfully.");
}

int handle_rmd(void)
{
  if (!qualify_validate(req_param)) return 1;
  if (rmdir(fullpath.s+1))
    return respond(550, 1, "Could not remove directory.");
  return respond(250, 1, "Directory removed successfully.");
}

int handle_dele(void)
{
  if (!qualify_validate(req_param)) return 1;
  if (unlink(fullpath.s+1))
    return respond(550, 1, "Could not remove file.");
  return respond(250, 1, "File removed successfully.");
}

int handle_rnfr(void)
{
  struct stat st;
  if (!qualify_validate(req_param)) return 1;
  if (stat(fullpath.s+1, &st) == -1) {
    if (errno == EEXIST)
      return respond(550, 1, "File does not exist.");
    else
      return respond(450, 1, "Could not locate file.");
  }
  if (!str_copy(&rnfr_filename, &fullpath)) return respond_internal_error();
  return respond(350, 1, "OK, file exists.");
}

int handle_rnto(void)
{
  int r;
  if (!qualify_validate(req_param)) return 1;
  if (!rnfr_filename.len) return respond(425, 1, "Send RNFR first.");
  r = rename(rnfr_filename.s+1, fullpath.s+1);
  str_truncate(&rnfr_filename, 0);
  if (r == -1) return respond(550, 1, "Could not rename file.");
  return respond(250, 1, "File renamed.");
}

int handle_site_chmod(void)
{
  unsigned mode;
  const char* ptr;
  for (ptr = req_param, mode = 0; *ptr >= '0' && *ptr <= '7'; ++ptr)
    mode = (mode * 8) | (*ptr - '0');
  if (ptr == req_param || *ptr != SPACE || mode > 0777)
    return respond(501, 1, "Invalid mode specification.");
  while (*ptr == SPACE) ++ptr;
  if (!qualify_validate(ptr)) return 1;
  if (chmod(fullpath.s+1, mode) != 0)
    return respond(550, 1, "Could not change modes on file.");
  return respond(250, 1, "File modes changed.");
}
