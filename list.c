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
#include <glob.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "systime.h"
#include "direntry.h"
#include "twoftpd.h"
#include "backend.h"

#ifndef GLOB_NOESCAPE
#define GLOB_NOESCAPE 0
#endif

static obuf out;

static char* buffer = 0;
static size_t buflen = 0;
static size_t bufsize = 0;
static const char** ptrs = 0;

static void listdir_append(const char* name)
{
  char* newbuf;
  size_t len;

  len = strlen(name) + 1;
  if (buflen + len > bufsize) {
    if (!bufsize) bufsize = 16;
    while (buflen + len > bufsize)
      bufsize *= 2;
    newbuf = malloc(bufsize);
    memcpy(newbuf, buffer, buflen);
    free(buffer);
    buffer = newbuf;
  }
  memcpy(buffer+buflen, name, len);
  buflen += len;
}

static int strpcmp(const void* a, const void* b)
{
  char** aa = (char**)a;
  char** bb = (char**)b;
  return strcmp(*aa, *bb);
}

static unsigned listdir(const char*** entries)
{
  unsigned count;
  unsigned i;
  DIR* dir;
  direntry* entry;
  const char* ptr;
  
  buflen = 0;
  count = 0;
  if ((dir = opendir(".")) == 0) return 0;
  while ((entry = readdir(dir)) != 0) {
    if (entry->d_name[0] == '.') continue;
    listdir_append(entry->d_name);
    ++count;
  }
  closedir(dir);

  if (ptrs) free(ptrs);
  ptrs = calloc(count+1, sizeof(char*));
  for (ptr = buffer, i = 0; i < count; i++) {
    ptrs[i] = ptr;
    ptr += strlen(ptr) + 1;
  }
  qsort(ptrs, count, sizeof(char*), strpcmp);
  ptrs[count] = 0;
  *entries = ptrs;
  return count;
}

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

static int output_line(const char* name)
{
  return obuf_puts(&out, name) && obuf_puts(&out, "\r\n");
}

static int list_single(const char* name)
{
  if (!make_out_connection(&out)) return 1;
  if (!output_line(name) || !obuf_flush(&out)) {
    obuf_close(&out);
    return respond(426, 1, "Transfer aborted.");
  }
  obuf_close(&out);
  return respond(226, 1, "Transfer complete.");
}

static int list_single_long(const char* name, const struct stat* stat)
{
  if (!make_out_connection(&out)) return 1;
  if (!output_stat(name, stat)) {
    obuf_close(&out);
    return respond(426, 1, "Transfer aborted.");
  }
  obuf_close(&out);
  return respond(226, 1, "Transfer complete.");
}

static int list_entries(const char** entries, unsigned count, int longfmt)
{
  struct stat statbuf;
  int result;
  
  if (!make_out_connection(&out)) return 1;

  for (; count; ++entries, --count) {
    if (longfmt) {
      if (stat(*entries, &statbuf) == -1) {
	if (errno == ENOENT) continue;
	obuf_close(&out);
	return respond(451, 1, "Error reading directory.");
      }
      result = output_stat(*entries, &statbuf);
    }
    else
      result = output_line(*entries);
    if (!result) {
      obuf_close(&out);
      return respond(426, 1, "Transfer aborted.");
    }
  }
  if (!obuf_flush(&out)) {
    obuf_close(&out);
    return respond(426, 1, "Transfer aborted.");
  }
  obuf_close(&out);
  return respond(226, 1, "Transfer complete.");
}

static int list_cwd(int longfmt)
{
  const char** entries;
  unsigned count;
  
  if ((count = listdir(&entries)) == 0)
    return respond(550, 1, "Could not list directory.");
  return list_entries(entries, count, longfmt);
}

static int list_dir(const char* path, int longfmt)
{
  int cwd;
  int result;
  if ((cwd = open(".", O_RDONLY)) == -1 ||
      chdir(path) == -1) {
    close(cwd);
    return respond(550, 1, "Could not list directory.");
  }
  result = list_cwd(longfmt);
  fchdir(cwd);
  close(cwd);
  return result;
}
    
int handle_listing(int longfmt)
{
  int result;
  struct stat statbuf;
  glob_t gfiles;
  const char* single;
  
  if (!req_param) return list_cwd(longfmt);
  
  single = 0;
  switch (glob(req_param, GLOB_MARK | GLOB_NOESCAPE, 0, &gfiles)) {
  case 0:
    break;
#ifdef GLOB_NOMATCH
  case GLOB_NOMATCH:
    single = req_param;
    break;
#endif
  default:
    return respond(550, 1, "Internal error while listing files.");
  }
  
  if (gfiles.gl_pathc == 1)
    single = gfiles.gl_pathv[0];
  if (single) {
    if (stat(single, &statbuf) == -1) {
      if (errno == EEXIST)
	result = respond(550, 1, "File or directory does not exist.");
      else
	result = respond(550, 1, "Could not access file.");
    }
    else if (S_ISDIR(statbuf.st_mode))
      result = list_dir(single, longfmt);
    else {
      if (longfmt)
	result = list_single_long(single, &statbuf);
      else
	result = list_single(single);
    }
  }
  else
    result = list_entries((const char**)gfiles.gl_pathv,
			  gfiles.gl_pathc, longfmt);
  globfree(&gfiles);
  return result;
}

int handle_list(void)
{
  return handle_listing(1);
}

int handle_nlst(void)
{
  return handle_listing(0);
}
