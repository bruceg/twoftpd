/* respond.c - twoftpd routines for responding to clients
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
#include "iobuf/iobuf.h"
#include "twoftpd.h"

int respond_start(unsigned code, int final)
{
  return obuf_putu(&outbuf, code) &&
    obuf_putc(&outbuf, final ? ' ' : '-');
}

int respond_end(void)
{
  return obuf_putsflush(&outbuf, "\r\n");
}

int respond_str(const char* str)
{
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

int respond(unsigned code, int final, const char* msg)
{
  return respond_start(code, final) &&
    respond_str(msg) &&
    respond_end();
}
