/* state.c - twoftpd routines for managing current server state
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
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

int binary_flag = 0;

int handle_type(void)
{
  if (!strcasecmp(req_param, "A") || !strcasecmp(req_param, "A N")) {
    binary_flag = 0;
    return respond(200, 1, "Transfer mode changed to ASCII.");
  }
  if (!strcasecmp(req_param, "I") || !strcasecmp(req_param, "L 8")) {
    binary_flag = 1;
    return respond(200, 1, "Transfer mode changed to BINARY.");
  }
  return respond(501, 1, "Unknown transfer type.");
}

int handle_stru(void)
{
  if (!strcasecmp(req_param, "F"))
    return respond(200, 1, "OK.");
  return respond(504, 1, "Invalid parameter.");
}

int handle_mode(void)
{
  if (!strcasecmp(req_param, "S"))
    return respond(200, 1, "OK.");
  return respond(504, 1, "Invalid parameter.");
}

int handle_cwd(void)
{
  if (chdir(req_param))
    return respond(550, 1, "Directory change failed.");
  show_message_file(250);
  return respond(250, 1, "Changed directory.");
}

int handle_pwd(void)
{
  size_t len;
  unsigned i;
  char buffer[BUFSIZE];

  if (!getcwd(buffer, sizeof buffer - 1))
    return respond(550, 1, "Could not determine current working directory.");
  len = strlen(buffer);
  buffer[len] = 0;
  for (i = 0; i < len; i++)
    if (buffer[len] == LF)
      buffer[len] = 0;
  return respond_start(257, 1) &&
    respond_str("\"") &&
    respond_str(buffer) &&
    respond_str("\"") &&
    respond_end();
}

int handle_cdup(void)
{
  if (chdir(".."))
    return respond(550, 1, "Directory change failed.");
  return respond(257, 1, "Changed directory.");
}
