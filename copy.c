/* copy.c - twoftpd network copy routine
 * Copyright (C) 2008  Bruce Guenter <bruce@untroubled.org>
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
#include "backend.h"

static int error_code(obuf* out)
{
  return (out->io.errnum == EPIPE
	  || out->io.errnum == ECONNRESET)
    ? 1
    : -1;
}

/* Return value:
 * -1 for a system error
 * 0 for a successful transfer
 */
static int copy_xlate(ibuf* in, obuf* out,
		      unsigned long (*xlate)(char* out,
					     const char* in,
					     unsigned long inlen),
		      unsigned long* bytes_in,
		      unsigned long* bytes_out)
{
  char in_buf[iobuf_bufsize];
  char out_buf[sizeof in_buf * 2];
  char* optr;
  unsigned long ocount;

  if (ibuf_error(in) || ibuf_error(out))
    return -1;
  *bytes_in = 0;
  *bytes_out = 0;
  for (;;) {
    if (!ibuf_read(in, in_buf, sizeof in_buf) && in->count == 0) {
      if (ibuf_eof(in))
	break;
      return -1;
    }
    *bytes_in += in->count;
    if (xlate) {
      optr = out_buf;
      ocount = xlate(out_buf, in_buf, in->count);
    }
    else {
      optr = in_buf;
      ocount = in->count;
    }
    if (!obuf_write(out, optr, ocount))
      return error_code(out);
    *bytes_out += ocount;
  }
  return 0;
}

int copy_xlate_close(ibuf* in, obuf* out,
		     unsigned long (*xlate)(char*, const char*, unsigned long),
		     unsigned long* bytes_in,
		     unsigned long* bytes_out)
{
  int status;
  status = copy_xlate(in, out, xlate, bytes_in, bytes_out);
  if (!ibuf_close(in))
    if (status == 0)
      status = -1;
  /* The close_out_connection adds an uncork operation, the results of
   * which are ignored for files. */
  if (!close_out_connection(out))
    if (status == 0)
      status = error_code(out);
  return status;
}
