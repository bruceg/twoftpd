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
#include "path/path.h"

int binary_flag = 0;
str cwd = {0,0,0};

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
  if (!strcasecmp(req_param, "F")) return respond_ok();
  return respond(504, 1, "Invalid parameter.");
}

int handle_mode(void)
{
  if (!strcasecmp(req_param, "S")) return respond_ok();
  return respond(504, 1, "Invalid parameter.");
}

int handle_cwd(void)
{
  struct stat statbuf;
  const char* fullpath;
  if ((fullpath = qualify(req_param)) == 0) return respond_internal_error();
  if (fullpath[0] != 0) {
    if (stat(fullpath, &statbuf) == -1)
      return respond(550, 1, "Directory does not exist.");
    if (!S_ISDIR(statbuf.st_mode))
      return respond(550, 1, "Is not a directory.");
    if (access(fullpath, R_OK|X_OK) == -1)
      return respond(550, 1, "Permission denied.");
  }
  if (!str_copys(&cwd, fullpath)) return respond_internal_error();
  show_message_file(250);
  return respond(250, 1, "Changed directory.");
}

int handle_pwd(void)
{
  return respond_start(257, 1) &&
    respond_str("\"") &&
    respond_str(cwd.s) &&
    respond_str("\"") &&
    respond_end();
}

int handle_cdup(void)
{
  req_param = "..";
  return handle_cwd();
}
