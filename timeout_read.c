/* timeout_read.c - Read data from a FD with a timeout.
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
#include <sys/poll.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

int timeout_read(int fd, char* buf, unsigned bufsize, unsigned* count)
{
  struct pollfd pf;
  unsigned rd;
  
  *count = 0;
  pf.fd = fd;
  pf.events = POLLIN;
  if (poll(&pf, 1, timeout * 1000) != 1) return 0;
  if ((rd = read(fd, buf, bufsize)) == -1) return 0;
  *count += rd;
  return 1;
}
