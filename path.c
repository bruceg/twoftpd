/* path.c - Routines for manipulating path strings
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
#include "backend.h"
#include "path/path.h"

static str fullpath;

const char* qualify(const char* path)
{
  if (!str_copy(&fullpath, &cwd)) return 0;
  if (!path_merge(&fullpath, path)) return 0;
  return fullpath.s+1;
}

int open_in(ibuf* in, const char* filename)
{
  if ((filename = qualify(filename)) == 0) return 0;
  return ibuf_open(in, filename, 0);
}

int open_out(obuf* out, const char* filename, int flags)
{
  if ((filename = qualify(filename)) == 0) return 0;
  return obuf_open(out, filename, flags, 0666, 0);
}
