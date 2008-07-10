/* respond.c - twoftpd routines for responding to clients
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
#include <string.h>
#include <iobuf/iobuf.h>
#include <systime.h>
#include "twoftpd.h"
#include "log.h"

extern int errno;
int log_responses = 0;

int respond_start(unsigned code, int final)
{
  const char* sep = final ? " " : "-";
  if (log_responses) {
    log_start();
    log_uint(code);
    log_str(sep);
  }
  return obuf_putu(&outbuf, code) && obuf_puts(&outbuf, sep);
}

int respond_end(void)
{
  if (log_responses) log_end();
  return obuf_putsflush(&outbuf, "\r\n");
}

int respond_str(const char* str)
{
  if (log_responses) log_str(str);
  /* Translate LF to NUL and \377 to \377\377 on output */
  for (; *str; ++str)
    switch (*str) {
    case LF:
      if (!obuf_putc(&outbuf, 0)) return 0;
      break;
    case '\377':
      if (!obuf_putc(&outbuf, '\377')) return 0;
      /* Fall through and output the \377 again */
    default:
      if (!obuf_putc(&outbuf, *str)) return 0;
    }
  return 1;
}

int respond_uint(unsigned long num)
{
  if (log_responses) log_uint(num);
  return obuf_putu(&outbuf, num);
}

int respond_syserr(unsigned code, const char *msg)
{
  return respond_start(code, 1) &&
    respond_str(msg) &&
    respond_str(": ") &&
    respond_str(strerror(errno)) &&
    respond_end();
}

int respond(unsigned code, int final, const char* msg)
{
  return respond_start(code, final) &&
    respond_str(msg) &&
    respond_end();
}

static struct timeval start;

void respond_start_xfer(void)
{
  gettimeofday(&start, 0);
}

static const char* scales[] = { "", "k", "M", "G", "T", 0 };

int respond_bytes(unsigned code,
		  const char* msg, unsigned long bytes, int sent)
{
  struct timeval end;
  unsigned long rate;
  int scaleno;
  gettimeofday(&end, 0);
  rate = bytes / (end.tv_sec-start.tv_sec +
		  (end.tv_usec-start.tv_usec)/1000000.0);
  for (scaleno = 0;
       rate > 10000 && scales[scaleno+1] != 0;
       ++scaleno, rate /= 1024)
    ;
  return respond_start(code, 1) &&
    respond_str(msg) &&
    respond_str(" (") &&
    respond_uint(bytes) &&
    respond_str(sent ? " bytes sent, " : " bytes received, ") &&
    respond_uint(rate) &&
    respond_str(scales[scaleno]) &&
    respond_str("B/s).") &&
    respond_end();
}
