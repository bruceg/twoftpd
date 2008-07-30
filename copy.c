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
#include <sysdeps.h>
#include <errno.h>
#include "backend.h"

static int error_code(obuf* out)
{
  return (out->io.errnum == EPIPE
	  || out->io.errnum == ECONNRESET)
    ? 2
    : -1;
}

/* Return value:
 * 0 if the iobuf was ready
 * 1 for timeout
 * 2 for an interrupt
 */
static int pollit(iobuf* io, int events, int timeout)
{
  iopoll_fd pfd[2];

  pfd[0].fd = 0;
  pfd[0].events = IOPOLL_READ;
  pfd[0].revents = 0;
  pfd[1].fd = io->fd;
  pfd[1].events = events;
  pfd[1].revents = 0;

  switch (iopoll_restart(pfd, 2, timeout)) {
  case -1:
    IOBUF_SET_ERROR(io);
    return -1;
  case 0:
    return 1;
  default:
    if (pfd[0].revents)
      return 2;
    return 0;
  }
}

/* Return value:
 * -1 for a system error
 * 0 for a successful transfer
 * 1 for timeout
 * 2 for interrupted transfer
 */
static int copy_xlate(ibuf* in, obuf* out, int timeout,
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
  int result;

  if (ibuf_error(in) || ibuf_error(out))
    return -1;
  *bytes_in = 0;
  *bytes_out = 0;
  for (;;) {
    if ((result = pollit(&in->io, IOPOLL_READ, timeout)) != 0)
      return result;
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
    if ((result = pollit(&out->io, IOPOLL_WRITE, timeout)) != 0)
      return result;
    if (!obuf_write(out, optr, ocount))
      return error_code(out);
    *bytes_out += ocount;
  }
  return 0;
}

int copy_xlate_close(ibuf* in, obuf* out, int timeout,
		     unsigned long (*xlate)(char*, const char*, unsigned long),
		     unsigned long* bytes_in,
		     unsigned long* bytes_out)
{
  int status;
  status = copy_xlate(in, out, timeout, xlate, bytes_in, bytes_out);
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
