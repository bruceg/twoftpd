/* retr.c - twoftpd routines for retrieving files
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
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

static unsigned long network_bytes;
unsigned long startpos = 0;

int handle_rest(void)
{
  const char* end;
  startpos = strtou(req_param, &end);
  if (*end != 0) {
    startpos = 0;
    return respond(501, 1, "Invalid number given for REST.");
  }
  return respond(350, 1, "Start position for transfer has been set.");
}

static int copy(ibuf* in, obuf* out)
{
  char in_buf[iobuf_bufsize];
  char out_buf[iobuf_bufsize*2];
  char* ptr;
  char* prev;
  char* optr;
  unsigned count;
  unsigned ocount;
  unsigned diff;

  if (ibuf_error(in)) return 0;
  network_bytes = 0;
  do {
    if (!ibuf_read(in, in_buf, sizeof in_buf) && in->count == 0) break;
    count = in->count;
    if (binary_flag) {
      optr = in_buf;
      ocount = count;
    }
    else {
      prev = in_buf;
      optr = out_buf;
      ocount = 0;
      for (;;) {
	if ((ptr = memchr(prev, LF, count)) == 0) break;
	diff = ptr - prev;
	memcpy(optr, prev, diff); optr += diff; ocount += diff;
	memcpy(optr, "\r\n", 2); optr += 2; ocount += 2;
	prev = ptr + 1;
	count -= diff + 1;
      }
      memcpy(optr, prev, count);
      ocount += count;
      optr = out_buf;
    }
    if (!obuf_write(out, optr, ocount)) return 0;
    network_bytes += ocount;
  } while (!ibuf_eof(in));
  if (!ibuf_eof(in)) return 0;
  return 1;
}
      
int handle_retr(void)
{
  int result;
  ibuf in;
  obuf out;
  unsigned long ss;
  
  ss = startpos;
  startpos = 0;
  if (!open_in(&in, req_param))
    return respond_syserr(550, "Could not open input file");
  if (ss && !ibuf_seek(&in, ss)) {
    ibuf_close(&in);
    return respond(550, 1, "Could not seek to start position in input file.");
  }
  if (!make_out_connection(&out)) {
    ibuf_close(&in);
    return 1;
  }
  result = copy(&in, &out);
  if (!ibuf_close(&in)) result = 0;
  if (!close_out_connection(&out)) result = 0;
  if (result)
    return respond_bytes(226, "File sent successfully", network_bytes, 1);
  else
    return respond_bytes(450, "Sending file failed", network_bytes, 1);
}
