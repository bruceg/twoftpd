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
#include "twoftpd.h"
#include "backend.h"
#include "path/path.h"

str fullpath;

static int validate_fullpath(void)
{
  if (nodotfiles) {
    long i;
    if (fullpath.s[0] == '.') return 0;
    for (i = str_findlast(&fullpath, '/'); i > 0;
	 i = str_findprev(&fullpath, '/', i-1))
      if (fullpath.s[i+1] == '.') return 0;
  }
  return 1;
}

static int qualify(const char* path)
{
  if (!str_copy(&fullpath, &cwd)) return 0;
  if (!path_merge(&fullpath, path)) return 0;
  return 1;
}

int qualify_validate(const char* path)
{
  if (!qualify(path)) {
    respond_internal_error();
    return 0;
  }
  if (!validate_fullpath()) {
    respond_permission_denied();
    return 0;
  }
  return 1;
}

int open_in(ibuf* in, const char* filename)
{
  if (!qualify(filename)) return 0;
  if (!validate_fullpath()) return 0;
  return ibuf_open(in, fullpath.s+1, 0);
}

int open_out(obuf* out, const char* filename, int flags)
{
  if (!qualify(filename)) return 0;
  if (!validate_fullpath()) return 0;
  return obuf_open(out, fullpath.s+1, flags, 0666, 0);
}
