/* listdir.c - twoftpd routines to list a directory
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
#include <stdlib.h>
#include "direntry.h"
#include "twoftpd.h"
#include "backend.h"

static char* buffer = 0;
static size_t buflen = 0;
static size_t bufsize = 0;
static const char** ptrs = 0;

static void append(const char* name)
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

const char** listdir(void)
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
    append(entry->d_name);
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
  return ptrs;
}
