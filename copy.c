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
#include <bglibs/sysdeps.h>
#include <errno.h>
#include <unistd.h>
#include <bglibs/socket.h>
#include "backend.h"

static int error_code(int err)
{
  return (err == EPIPE
	  || err == ECONNRESET)
    ? 2
    : -1;
}

/* Return value:
 * 0 if the iobuf was ready
 * 1 for timeout
 * 2 for an interrupt
 */
static int pollit(int fd, int events, int timeout)
{
  iopoll_fd pfd[2];

  pfd[0].fd = 0;
  pfd[0].events = IOPOLL_READ;
  pfd[0].revents = 0;
  pfd[1].fd = fd;
  pfd[1].events = events;
  pfd[1].revents = 0;

  switch (iopoll_restart(pfd, 2, timeout)) {
  case -1:
    return -1;
  case 0:
    return 1;
  default:
    if (pfd[0].revents)
      return 2;
    return 0;
  }
}

/* Return values for following routines:
 * -1 for a system error
 * 0 for a successful transfer
 * 1 for timeout
 * 2 for interrupted transfer
 */

int netwrite(int out, const char* optr, unsigned long ocount, int timeout)
{
  int result;
  ssize_t wr;

  while (ocount > 0) {
    if ((result = pollit(out, IOPOLL_WRITE, timeout)) != 0)
      return result;
    if ((wr = write(out, optr, ocount)) == -1)
      return error_code(errno);
    if (wr == 0) {
      errno = EIO;
      return -1;
    }
    ocount -= wr;
    optr += wr;
  }
  return 0;
}

static int copy_xlate(int in, int out, int timeout,
		      unsigned long (*xlate)(char* out,
					     const char* in,
					     unsigned long inlen),
		      unsigned long* bytes_in,
		      unsigned long* bytes_out)
{
  char in_buf[iobuf_bufsize];
  char out_buf[sizeof in_buf * 2];
  char* optr;
  ssize_t icount;
  ssize_t ocount;
  int result;

  *bytes_in = 0;
  *bytes_out = 0;
  for (;;) {
    if ((result = pollit(in, IOPOLL_READ, timeout)) != 0)
      return result;
    if ((icount = read(in, in_buf, sizeof in_buf)) == -1)
      return -1;
    if (icount == 0)
      return 0;
    *bytes_in += icount;
    if (xlate) {
      optr = out_buf;
      ocount = xlate(out_buf, in_buf, icount);
    }
    else {
      optr = in_buf;
      ocount = icount;
    }
    if ((result = netwrite(out, optr, ocount, timeout)) != 0)
      return result;
    *bytes_out += ocount;
  }
  return 0;
}

int copy_xlate_close(int in, int out, int timeout,
		     unsigned long (*xlate)(char*, const char*, unsigned long),
		     unsigned long* bytes_in,
		     unsigned long* bytes_out)
{
  int status;
  status = copy_xlate(in, out, timeout, xlate, bytes_in, bytes_out);
  /* Three possible cases here: in is at EOF, an error occurred while
   * reading in, or writing is completed.  In all cases we can ignore
   * errors during closing in. */
  close(in);
  /* The output might be a socket that needs to be uncorked.  If the
   * output is a file, this is harmless. */
  socket_uncork(out);
  if (close(out) != 0)
    if (status == 0)
      status = error_code(errno);
  return status;
}
