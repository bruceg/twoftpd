/* list.c - twoftpd routines to handle list/nlst commands
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <systime.h>
#include <sysdeps.h>
#include "twoftpd.h"
#include "backend.h"
#include <str/str.h>
#include <path/path.h>

int list_options;

static int list_long;
static int list_flags;
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
    (mode & 0001) ? 't' : 'T' :
    (mode & 0001) ? 'x' : '-';
  buf[10] = 0;
  return buf;
}

static int str_catmode(str* s, int mode)
{
  return str_catb(s, mode2str(mode), 10);
}

static int str_catowner(str* s, uid_t owner)
{
  const char* name;
  unsigned len;
  if (owner == uid)
    name = user, len = user_len;
  else
    name = "somebody", len = 8;
  if (!str_catb(s, name, len)) return 0;
  while (len++ < MAX_NAME_LEN)
    if (!str_catc(s, SPACE)) return 0;
  return 1;
}

static int str_catgroup(str* s, gid_t g)
{
  const char* name;
  unsigned len;
  if (g == gid)
    name = group, len = group_len;
  else
    name = "somegrp", len = 7;
  if (!str_catb(s, name, len)) return 0;
  while (len++ < MAX_NAME_LEN)
    if (!str_catc(s, SPACE)) return 0;
  return 1;
}

static int str_cattime(str* s, time_t then)
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
  return str_catb(s, buf, len);
}

static int str_catstat(str* s, const struct stat* st)
{
  if (st)
    return str_catmode(s, st->st_mode) &&
      str_catb(s, "    1 ", 6) &&
      str_catowner(s, st->st_uid) &&
      str_catc(s, SPACE) &&
      str_catgroup(s, st->st_gid) &&
      str_catc(s, SPACE) &&
      str_catuwll(s, st->st_size, 8, ' ') &&
      str_catc(s, SPACE) &&
      str_cattime(s, st->st_mtime) &&
      str_catc(s, SPACE);
  else
    return str_cats(s,
		    "??????????"
		    "    ? "
		    "???????? "
		    "???????? "
		    "       ? "
		    "??? ?? ????? ");
}

static int str_catflags(str* s, const struct stat* st)
{
  if (st) {
    int ch;
    int mode = st->st_mode;
    ch = 0;
    if (S_ISDIR(mode)) ch = '/';
    else if (S_ISFIFO(mode)) ch = '|';
    else if (S_ISSOCK(mode)) ch = '=';
    else if (list_flags == 'F' && mode & 0111) ch = '*';
    if (ch)
      return str_catc(s, ch);
  }
  return 1;
}

static int str_catfn(str* s, const char* fn, int striplen)
{
  if (striplen < 0) {
    if (!str_catc(s, '/')) return 0;
    striplen = 0;
  }
  return str_cats(s, fn + striplen);
}

static int respond_writecode(int code, unsigned long bytes_out)
{
  switch (code) {
  case 0:
    return respond_bytes(226, "Listing completed", bytes_out, 1);
  case 1:
    return respond_bytes(426, "Listing timed out", bytes_out, 1);
  case 2:
    return respond_bytes(426, "Listing interrupted", bytes_out, 1);
  default:
    return respond_bytes(451, "Listing failed", bytes_out, 1);
  }
}

static char send_buf[8192];
static unsigned long send_used;

static int send_line(int out, const str* s)
{
  int result;

  if (send_used + s->len > sizeof send_buf) {
    if ((result = netwrite(out, send_buf, send_used, timeout * 1000)) != 0)
      return result;
    send_used = 0;
  }
  if (s->len > sizeof send_buf)
    return netwrite(out, s->s, s->len, timeout * 1000);
  memcpy(send_buf + send_used, s->s, s->len);
  send_used += s->len;
  return 0;
}

static str entries;

static int list_entries(long count, int striplen)
{
  static str line;
  obuf out;
  int result;
  struct stat statbuf;
  struct stat* statptr;
  const char* filename = entries.s;
  int need_stat = list_long || list_flags;
  
  if (mode_nlst && count < 0)
    return respond(550, 1, "No such file or directory");
  
  if (!make_out_connection(&out)) return 1;
  out.io.timeout = timeout * 1000;

  for (; count > 0; --count, filename += strlen(filename)+1) {
    statptr = 0;
    if (need_stat) {
      if (stat(filename, &statbuf) == -1) {
	if (errno == ENOENT) continue;
      }
      else
	statptr = &statbuf;
    }
    line.len = 0;
    if ((list_long && !str_catstat(&line, statptr))
	|| !str_catfn(&line, filename, striplen)
	|| (list_flags && !str_catflags(&line, statptr))
	|| !str_cats(&line, CRLF)) {
      close_out_connection(&out);
      return respond_bytes(451, "Internal error", out.io.offset, 1);
    }
    if ((result = send_line(out.io.fd, &line)) != 0)
      return respond_writecode(result, out.io.offset);
    out.io.offset += line.len;
  }
  if (send_used > 0) {
    if ((result = netwrite(out.io.fd, send_buf, send_used, timeout * 1000)) != 0) {
      close_out_connection(&out);
      return respond_writecode(result, out.io.offset);
    }
  }
  if (!close_out_connection(&out))
    return respond_bytes(426, "Listing aborted", out.io.offset, 1);
  return respond_bytes(226, "Listing complete", out.io.offset, 1);
}

static int list_dir()
{
  long count;
  DIR* dir;
  direntry* entry;

  /* Turning "/" into "/." simplifies several other steps below. */
  if (fullpath.len == 1)
    if (!str_copys(&fullpath, "/."))
      return respond_internal_error();

  if ((dir = opendir(fullpath.s+1)) == 0)
    return respond_syserr(550, "Could not open directory");
  entries.len = 0;
  count = 0;
  while ((entry = readdir(dir)) != 0) {
    /* Always skip the "." and ".." entries.  Skip all other ".*"
     * entries if nodotfiles was set. */
    if (entry->d_name[0] != '.'
	|| (!(entry->d_name[1] == 0
	      || (entry->d_name[1] == '.' && entry->d_name[2] == 0))
	    && (list_options & PATH_MATCH_DOTFILES) != 0)) {
      /* Add the entry to the list, skipping the "/" separator if none
       * is needed */
      if (!str_catb(&entries, fullpath.s+1, fullpath.len-1)
	  || !str_catc(&entries, '/')
	  || !str_catb(&entries, entry->d_name, strlen(entry->d_name)+1)) {
	closedir(dir);
	return respond_internal_error();
      }
      ++count;
    }
  }
  closedir(dir);
  if (!str_sort(&entries, 0, count, 0))
    return respond_internal_error();
  return list_entries(count, fullpath.len);
}

static int handle_listing(int longfmt)
{
  int result;
  struct stat statbuf;
  long count;
  int striplen;
  
  mode_nlst = !longfmt;
  list_long = longfmt;
  list_flags = 0;
  
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
	case 'F':
	case 'p':
	  list_flags = *req_param; break;
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

  if (!req_param || !*req_param)
    req_param = ".";
  
  if (path_contains(req_param, ".."))
    return respond(553, 1, "Paths containing '..' not allowed.");

  /* Prefix the requested path with CWD, and strip it after */
  if (!qualify_validate(req_param)) return 1;
  if (fullpath.len == 1) return list_dir();
  if ((count = path_match(fullpath.s+1, &entries, list_options)) == -1)
    return respond_internal_error();
  striplen = (req_param[0] == '/') ? -1 : (cwd.len == 1) ? 0 : cwd.len;
  
  if (count <= 1) {
    /* If no entries were matched, try the literal path. */
    if (count == 0)
      if (!str_copyb(&entries, fullpath.s+1, fullpath.len))
	return respond_internal_error();

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
  return handle_listing(1);
}

int handle_nlst(void)
{
  return handle_listing(0);
}
