/* path.c - Routines for manipulating path strings
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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"
#include <path/path.h>

str fullpath;

static int validate_fullpath(void)
{
  errno = EPERM;
  if (lockhome) {
    unsigned long homelen = strlen(home);
    if (memcmp(fullpath.s, home, homelen) != 0 ||
	(fullpath.s[homelen] != 0 && fullpath.s[homelen] != '/'))
      return 0;
  }
  if (nodotfiles) {
    long i;
    if (fullpath.s[0] == '.') return 0;
    if (fullpath.s[0] == '/' && fullpath.s[1] == '.') return 0;
    for (i = str_findlast(&fullpath, '/'); i > 0;
	 i = str_findprev(&fullpath, '/', i-1))
      if (fullpath.s[i+1] == '.') return 0;
  }
  errno = 0;
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

int open_fd(const char* filename, int flags, int mode)
{
  int fd;
  struct stat st;
  if (!qualify(filename)) return -1;
  if (!validate_fullpath()) return -1;
  if ((fd = open(fullpath.s+1, flags, mode)) == -1) return -1;
  if (fstat(fd, &st) == 0) {
    if (S_ISREG(st.st_mode))
      return fd;
    else
      errno = EPERM;
  }
  close(fd);
  return -1;
}

int open_in(ibuf* in, const char* filename)
{
  int fd;
  if ((fd = open_fd(filename, O_RDONLY, 0)) == -1) return 0;
  if (ibuf_init(in, fd, 0, IOBUF_NEEDSCLOSE, 0)) return 1;
  close(fd);
  return 0;
}
