/* timeout_write.c - Write data to a FD with a timeout.
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

int timeout_write(int fd, char* buf, unsigned bufsize)
{
  struct pollfd pf;
  unsigned wr;
  
  pf.fd = fd;
  pf.events = POLLIN;
  while (bufsize) {
    if (poll(&pf, 1, timeout * 1000) != 1) return 0;
    for (;;) {
      if ((wr = write(fd, buf, bufsize)) == -1 || wr == 0) return 0;
      buf += wr;
      bufsize -= wr;
    }
  }
  return 1;
}
