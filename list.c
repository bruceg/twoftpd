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

int list_options;

static obuf out;
static int list_long;
static int mode_nlst;

static const char* mode2str(int mode)
{
  static char buf[11];
  buf[0] = S_ISREG(mode) ? '-' :
    S_ISDIR(mode) ? 'd' :
    S_ISCHR(mode) ? 'c' :
    S_ISBLK(mode) ? 'b' :
    S_ISFIFO(mode) ? 'p' :
    S_ISSOCK(mode) ? 's' :
    '?';
  buf[1] = (mode & 0400) ? 'r' : '-';
  buf[2] = (mode & 0200) ? 'w' : '-';
  buf[3] = (mode & 04000) ?
    (mode & 0100) ? 's' : 'S' :
    (mode & 0100) ? 'x' : '-';
  buf[4] = (mode & 0040) ? 'r' : '-';
  buf[5] = (mode & 0020) ? 'w' : '-';
  buf[6] = (mode & 02000) ?
    (mode & 0010) ? 's' : 'S' :
    (mode & 0010) ? 'x' : '-';
  buf[7] = (mode & 0004) ? 'r' : '-';
  buf[8] = (mode & 0002) ? 'w' : '-';
  buf[9] = (mode & 01000) ?
    (mode & 0001) ? 's' : 'S' :
    (mode & 0001) ? 'x' : '-';
  buf[10] = 0;
  return buf;
}

static int output_mode(int mode)
{
  return obuf_write(&out, mode2str(mode), 10);
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

static int output_group(gid_t g)
{
  unsigned i;
  if (g == gid) {
    if (!obuf_write(&out, group, group_len > 8 ? 8 : group_len)) return 0;
    for (i = group_len; i < 8; i++)
      if (!obuf_putc(&out, SPACE)) return 0;
  }
  else
    if (!obuf_write(&out, "somegrp ", 8)) return 0;
  return 1;
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

static int list_entries(long count, unsigned striplen)
{
  struct stat statbuf;
  int result;
  const char* filename = entries.s;

  if (mode_nlst && count < 0)
    return respond(550, 1, "No such file or directory");
  
  if (!make_out_connection(&out)) return 1;

  for (; count > 0; --count, filename += strlen(filename)+1) {
    if (list_long) {
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
      close_out_connection(&out);
      return respond_bytes(426, "Listing aborted", out.io.offset, 1);
    }
  }
  if (!close_out_connection(&out))
    return respond_bytes(426, "Listing aborted", out.io.offset, 1);
  return respond_bytes(226, "Listing complete", out.io.offset, 1);
}

static int list_dir()
{
  long count;
  if (!path_merge(&fullpath, "*") ||
      (count = path_match(fullpath.s+1, &entries, list_options)) == -1)
    return respond_internal_error();
  return list_entries(count, str_findlast(&fullpath, '/'));
}

static int list_cwd()
{
  if (!str_copy(&fullpath, &cwd))
    return respond_internal_error();
  return list_dir();
}

int handle_listing()
{
  int result;
  struct stat statbuf;
  long count;
  long striplen;
  
  if (req_param) {
    while (*req_param == '-') {
      ++req_param;
      if (*req_param == SPACE) break;
      while (*req_param && *req_param != SPACE) {
	char tmp[2];
	switch (*req_param) {
	case 'a': break;	/* Listing all files is */
	case 'A': break;	/* not controlled by client */
	case 'L': break;	/* We already dereference symlinks */
	case 'l': list_long = 1; break;
	default:
	  tmp[0] = *req_param;
	  tmp[1] = 0;
	  return respond_start(550, 1) &&
	    respond_str("Unknown list option: ") &&
	    respond_str(tmp) && respond_end();
	}
	++req_param;
      }
      if (*req_param == SPACE) ++req_param;
    }
  }

  if (!req_param || !*req_param) return list_cwd();
  
  if (path_contains(req_param, ".."))
    return respond(553, 1, "Paths containing '..' not allowed.");

  /* Prefix the requested path with CWD, and strip it after */
  if (!qualify_validate(req_param)) return 1;
  if (fullpath.len == 1) return list_cwd();
  if ((count = path_match(fullpath.s+1, &entries, list_options)) == -1)
    return respond_internal_error();
  striplen = (cwd.len == 1) ? 0 : cwd.len;
  
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
      return list_dir();
    }
  }
  return list_entries(count, striplen);
}

int handle_list(void)
{
  mode_nlst = 0;
  list_long = 1;
  return handle_listing();
}

int handle_nlst(void)
{
  mode_nlst = 1;
  list_long = 0;
  return handle_listing();
}
