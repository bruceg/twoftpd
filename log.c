/* log.c - Log message generation routines
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
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <iobuf/iobuf.h>
#include "log.h"

static pid_t pid = 0;

void log_start(void)
{
  if (!pid) pid = getpid();
  obuf_puts(&errbuf, program);
  obuf_putc(&errbuf, '[');
  obuf_putu(&errbuf, pid);
  obuf_puts(&errbuf, "]: ");
}

void log1(const char* a)
{
  log_start();
  log_str(a);
  log_end();
}

void log2(const char* a, const char* b)
{
  log_start();
  log_str(a);
  log_str(" ");
  log_str(b);
  log_end();
}
