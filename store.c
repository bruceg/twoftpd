/* store.c - twoftpd routines for handling storage-modifying commands
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
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

static str rnfr_filename;

static unsigned long xlate_ascii(char* out,
				 const char* in,
				 unsigned long inlen)
{
  unsigned long outlen;
  for (outlen = 0; inlen > 0; --inlen, ++in) {
    if (*in == CR)
      --inlen, ++in;
    *out = *in;
    ++outlen;
    ++out;
  }
  return outlen;
}

static int open_copy_close(int append)
{
  int r;
  ibuf in;
  obuf out;
  unsigned long ss;
  unsigned long bytes_in;
  unsigned long bytes_out;
  
  ss = startpos;
  startpos = 0;
  if (!open_out(&out, req_param,
		append ? O_APPEND
		: store_exclusive ? O_CREAT | O_EXCL
		: ss ? 0
		: O_CREAT | O_TRUNC))
    return respond_syserr(550, "Could not open output file");
  if (ss && !obuf_seek(&out, ss)) {
    obuf_close(&out);
    return respond(550, 1, "Could not seek to start position in output file.");
  }
  if (!make_in_connection(&in)) {
    obuf_close(&out);
    if (store_exclusive)
      unlink(req_param);
    return 1;
  }
  r = copy_xlate_close(&in, &out, binary_flag ? 0 : xlate_ascii,
		       &bytes_in, &bytes_out);
  if (r == 0)
    return respond_bytes(226, "File received successfully", bytes_in, 0);
  else
    return respond_bytes(451, "File store failed", bytes_in, 0);
}

int handle_stor(void)
{
  return open_copy_close(0);
}

int handle_appe(void)
{
  return open_copy_close(1);
}

int handle_mkd(void)
{
  if (!qualify_validate(req_param)) return 1;
  if (mkdir(fullpath.s+1, 0777))
    return respond_syserr(550, "Could not create directory");
  return respond(257, 1, "Directory created successfully.");
}

int handle_rmd(void)
{
  if (!qualify_validate(req_param)) return 1;
  if (rmdir(fullpath.s+1))
    return respond_syserr(550, "Could not remove directory");
  return respond(250, 1, "Directory removed successfully.");
}

int handle_dele(void)
{
  if (!qualify_validate(req_param)) return 1;
  if (unlink(fullpath.s+1))
    return respond_syserr(550, "Could not remove file");
  return respond(250, 1, "File removed successfully.");
}

int handle_rnfr(void)
{
  struct stat st;
  if (!qualify_validate(req_param)) return 1;
  if (stat(fullpath.s+1, &st) == -1) {
    if (errno == EEXIST)
      return respond(550, 1, "File does not exist.");
    else
      return respond(450, 1, "Could not locate file.");
  }
  if (!str_copy(&rnfr_filename, &fullpath)) return respond_internal_error();
  return respond(350, 1, "OK, file exists.");
}

int handle_rnto(void)
{
  int r;
  if (!qualify_validate(req_param)) return 1;
  if (!rnfr_filename.len) return respond(425, 1, "Send RNFR first.");
  r = rename(rnfr_filename.s+1, fullpath.s+1);
  str_truncate(&rnfr_filename, 0);
  if (r == -1) return respond_syserr(550, "Could not rename file");
  return respond(250, 1, "File renamed.");
}

int handle_site_chmod(void)
{
  unsigned mode;
  const char* ptr;
  for (ptr = req_param, mode = 0; *ptr >= '0' && *ptr <= '7'; ++ptr)
    mode = (mode * 8) | (*ptr - '0');
  if (ptr == req_param || *ptr != SPACE || mode > 0777)
    return respond(501, 1, "Invalid mode specification.");
  while (*ptr == SPACE) ++ptr;
  if (!qualify_validate(ptr)) return 1;
  if (chmod(fullpath.s+1, mode) != 0)
    return respond_syserr(550, "Could not change modes on file");
  return respond(250, 1, "File modes changed.");
}
