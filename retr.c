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
  char* ptr;
  char* prev;
  unsigned count;
  
  if (ibuf_eof(in)) return 1;
  if (ibuf_error(in) || obuf_error(out)) return 0;
  do {
    if (!ibuf_read(in, buf, sizeof buf) && in->count == 0) break;
    count = in->count;
    prev = buf;
    if (!binary_flag) {
      for(;;) {
	if ((ptr = memchr(prev, LF, count)) == 0) break;
	if (!obuf_write(out, prev, ptr - prev)) return 0;
	if (!obuf_write(out, "\r\n", 2)) return 0;
	prev = ptr + 1;
	count = buf + in->count - prev;
      }
    }
    if (!obuf_write(out, prev, count)) return 0;
  } while (!ibuf_eof(in));
  if (!ibuf_eof(in)) return 0;
  if (!obuf_flush(out)) return 0;
  return 1;
}
      
int handle_retr(void)
{
  ibuf in;
  obuf out;
  int result;
  
  if (!ibuf_open(&in, req_param, 0))
    return respond(550, 1, "Could not open input file.");
  if (!make_out_connection(&out)) {
    ibuf_close(&in);
    return 1;
  }
  result = copy(&in, &out);
  ibuf_close(&in);
  obuf_close(&out);
  return result ? respond(226, 1, "File sent successfully.") : 1;
}
