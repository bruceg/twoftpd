/* stat.c - twoftpd routines for producing file status
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
#include <stdio.h>
#include "systime.h"
#include "twoftpd.h"
#include "backend.h"

int handle_size(void)
{
  struct stat statbuf;
  char buffer[40];
  const char* fullpath;
  
  if ((fullpath = qualify(req_param)) == 0) return respond_internal_error();
  if (stat(fullpath, &statbuf) == -1)
    return respond(550, 1, "Could not determine file size.");
  snprintf(buffer, sizeof buffer, "%lu", statbuf.st_size);
  return respond(213, 1, buffer);
}

int handle_mdtm(void)
{
  struct stat statbuf;
  char buffer[16];
  const char* fullpath;
  
  if ((fullpath = qualify(req_param)) == 0) return respond_internal_error();
  if (stat(fullpath, &statbuf) == -1)
    return respond(550, 1, "Could not determine file time.");
  strftime(buffer, sizeof buffer, "%Y%m%d%H%M%S",
	   gmtime(&statbuf.st_mtime));
  return respond(213, 1, buffer);
}
