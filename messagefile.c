/* messagefile.c - Show a message file to the client.
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
#include <stdlib.h>
#include <string.h>
#include <bglibs/systime.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

const char* message_file;

void show_message_file(unsigned code)
{
  int fd;
  ibuf in;
  char buf[1024];

  if (message_file) {
    if ((fd = open_fd(message_file, O_RDONLY, 0)) != -1) {
      if (ibuf_init(&in, fd, 0, 0, 0)) {
	while (ibuf_gets(&in, buf, sizeof buf, '\n')) {
	  if (in.count > 1) {
	    if (buf[in.count-1] == '\n')
	      buf[in.count-1] = 0;
	    respond(code, 0, buf);
	  }
	}
      }
      ibuf_close(&in);
    }
    close(fd);
  }
}
