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

static int copy(ibuf* in, obuf* out)
{
  char buf[iobuf_bufsize];
  char obuf[iobuf_bufsize*2];
  char* ptr;
  char* prev;
  char* optr;
  unsigned count;
  unsigned ocount;
  unsigned diff;
  
  if (ibuf_error(in)) return 0;
  do {
    if (!ibuf_read(in, buf, sizeof buf) && in->count == 0) break;
    count = in->count;
    if (binary_flag) {
      optr = buf;
      ocount = count;
    }
    else {
      prev = buf;
      optr = obuf;
      ocount = 0;
      for (;;) {
	if ((ptr = memchr(prev, LF, count)) == 0) break;
	diff = ptr - prev;
	memcpy(optr, prev, diff); optr += diff; ocount += diff;
	memcpy(optr, "\r\n", 2); optr += 2; ocount += 2;
	prev = ptr + 1;
	count -= diff + 1;
      }
      optr = obuf;
    }
    if (!obuf_write(out, optr, ocount)) return 0;
  } while (!ibuf_eof(in));
  if (!ibuf_eof(in)) return 0;
  return 1;
}
      
int handle_retr(void)
{
  int result;
  ibuf in;
  obuf out;
  
  if (!ibuf_open(&in, req_param, 0))
    return respond(550, 1, "Could not open input file.");
  if (!make_out_connection(&out)) {
    ibuf_close(&in);
    return 1;
  }
  result = copy(&in, &out);
  ibuf_close(&in);
  if (!obuf_close(&out)) result = 0;
  return result ?
    respond(226, 1, "File sent successfully.") :
    respond(450, 1, "Sending file failed.");
}
