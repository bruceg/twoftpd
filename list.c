/* list.c - twoftpd routines to handle list/nlst commands
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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "systime.h"
#include "direntry.h"
#include "twoftpd.h"
#include "backend.h"
#include "str/str.h"
#include "path/path.h"

static obuf out;

static int output_mode(int mode)
{
  return obuf_write(&out, S_ISDIR(mode) ? "drwxr-xr-x" :
		    S_ISREG(mode) ? "-rw-r--r--" : "??????????", 10);
}

static int output_owner(uid_t owner)
{
  unsigned i;
  if (owner == uid) {
    if (!obuf_write(&out, user, user_len > 8 ? 8 : user_len)) return 0;
    for (i = user_len; i < 8; i++)
      if (!obuf_putc(&out, SPACE)) return 0;
  }
  else
    if (!obuf_write(&out, "somebody", 8)) return 0;
  return 1;
}

static int output_group(gid_t group)
{
  return obuf_write(&out, (group == gid) ? "group   " : "somegrp ", 8);
}

static int output_time(time_t then)
{
  struct tm* tm;
  time_t year;
  char buf[16];
  unsigned len;
  
  year = now - 365/2*24*60*60;

  tm = localtime(&then);
  if (then > year)
    len = strftime(buf, sizeof buf - 1, "%b %e %H:%M", tm);
  else
    len = strftime(buf, sizeof buf - 1, "%b %e  %Y", tm);
  return obuf_write(&out, buf, len);
}

static int output_stat(const char* filename, const struct stat* s)
{
  return output_mode(s->st_mode) &&
    obuf_write(&out, "    1 ", 6) &&
    output_owner(s->st_uid) &&
    obuf_putc(&out, SPACE) &&
    output_group(s->st_gid) &&
    obuf_putc(&out, SPACE) &&
    obuf_putuw(&out, s->st_size, 8, ' ') &&
    obuf_putc(&out, SPACE) &&
    output_time(s->st_mtime) &&
    obuf_putc(&out, SPACE) &&
    obuf_put2s(&out, filename, "\r\n");
}

static int output_staterr(const char* filename)
{
  return obuf_put3s(&out, "??????????"
		    "    ? "
		    "???????? "
		    "???????? "
		    "       ? "
		    "??? ?? ????? ",
		    filename, "\r\n");
}

static int output_line(const char* name)
{
  return obuf_puts(&out, name) && obuf_puts(&out, "\r\n");
}

static str entries;

static int list_entries(long count, unsigned striplen, int longfmt)
{
  struct stat statbuf;
  int result;
  const char* filename = entries.s;
  
  if (!make_out_connection(&out)) return 1;

  if (count < 0) {
    result = obuf_putstr(&out, &fullpath) &&
      obuf_puts(&out, ": No such file or directory.\r\n");
  }
  else {
    for (; count; --count, filename += strlen(filename)+1) {
      if (longfmt) {
	if (stat(filename, &statbuf) == -1) {
	  if (errno == ENOENT) continue;
	  result = output_staterr(filename+striplen);
	}
	else
	  result = output_stat(filename+striplen, &statbuf);
      }
      else
	result = output_line(filename+striplen);
      if (!result) {
	obuf_close(&out);
	return respond(426, 1, "Transfer aborted.");
      }
    }
  }
  if (!obuf_flush(&out)) {
    obuf_close(&out);
    return respond(426, 1, "Transfer aborted.");
  }
  obuf_close(&out);
  return respond(226, 1, "Transfer complete.");
}

static int list_dir(int longfmt, unsigned options)
{
  long count;
  if (!path_merge(&fullpath, "*") ||
      (count = path_match(fullpath.s+1, &entries, options)) == -1)
    return respond_internal_error();
  return list_entries(count, str_findlast(&fullpath, '/'), longfmt);
}

static int list_cwd(int longfmt, unsigned options)
{
  if (!str_copy(&fullpath, &cwd))
    return respond_internal_error();
  return list_dir(longfmt, options);
}

int handle_listing(int longfmt)
{
  int result;
  struct stat statbuf;
  long count;
  long striplen;
  unsigned options = 0;
  
  if (!nodotfiles) options |= PATH_MATCH_DOTFILES;
  
  if (!req_param) return list_cwd(longfmt, options);
  
  if (path_contains(req_param, ".."))
    return respond(553, 1, "Paths containing '..' not allowed.");

  /* Prefix the requested path with CWD, and strip it after */
  if (!qualify_validate(req_param)) return 1;
  if ((count = path_match(fullpath.s+1, &entries, options)) == -1)
    return respond_internal_error();
  striplen = cwd.len;
  
  if (count == 0)
    count = -1;
  else if (count == 1) {
    if (stat(entries.s, &statbuf) == -1) {
      if (errno == EEXIST)
	result = respond(550, 1, "File or directory does not exist.");
      else
	result = respond(550, 1, "Could not access file.");
    }
    else if (S_ISDIR(statbuf.st_mode)) {
      if (!str_copys(&fullpath, "/") ||
	  !str_catb(&fullpath, entries.s, entries.len-1))
	return respond_internal_error();
      return list_dir(longfmt, options);
    }
  }
  return list_entries(count, striplen, longfmt);
}

int handle_list(void)
{
  return handle_listing(1);
}

int handle_nlst(void)
{
  return handle_listing(0);
}
