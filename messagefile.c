/* messagefile.c - Show a message file to the client.
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
#include <stdlib.h>
#include <string.h>
#include <systime.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

const char* message_file;

void show_message_file(unsigned code)
{
  ibuf in;
  char buf[1024];
  if (!message_file) return;
  if (!open_in(&in, message_file)) return;
  while (ibuf_gets(&in, buf, sizeof buf, '\n')) {
    if (buf[in.count-1] == '\n')
      buf[in.count-1] = 0;
    respond(code, 0, buf);
  }
  ibuf_close(&in);
}
