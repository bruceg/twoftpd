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
#include <systime.h>
#include <fmt/number.h>
#include "twoftpd.h"
#include "backend.h"

int handle_size(void)
{
  struct stat statbuf;
  char buffer[40];

  if (!qualify_validate(req_param)) return 1;
  if (stat(fullpath.s+1, &statbuf) == -1)
    return respond_syserr(550, "Could not determine file size");
  buffer[fmt_ulldec(buffer, statbuf.st_size)] = 0;
  return respond(213, 1, buffer);
}

int handle_mdtm(void)
{
  struct stat statbuf;
  char buffer[16];
  
  if (!qualify_validate(req_param)) return 1;
  if (stat(fullpath.s+1, &statbuf) == -1)
    return respond_syserr(550, "Could not determine file time");
  strftime(buffer, sizeof buffer, "%Y%m%d%H%M%S",
	   gmtime(&statbuf.st_mtime));
  return respond(213, 1, buffer);
}
