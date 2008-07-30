/* retr.c - twoftpd routines for retrieving files
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
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

unsigned long startpos = 0;

int handle_rest(void)
{
  const char* end;
  startpos = strtou(req_param, &end);
  if (*end != 0) {
    startpos = 0;
    return respond(501, 1, "Invalid number given for REST.");
  }
  return respond(350, 1, "Start position for transfer has been set.");
}

static unsigned long xlate_ascii(char* out,
				 const char* in,
				 unsigned long inlen)
{
  unsigned long outlen;
  for (outlen = 0; inlen > 0; --inlen, ++in) {
    if (*in == LF) {
      *out++ = CR;
      ++outlen;
    }
    *out++ = *in;
    ++outlen;
  }
  return outlen;
}

int handle_retr(void)
{
  int result;
  ibuf in;
  obuf out;
  unsigned long ss;
  struct stat st;
  unsigned long bytes_in;
  unsigned long bytes_out;
  
  ss = startpos;
  startpos = 0;
  if (!open_in(&in, req_param))
    return respond_syserr(550, "Could not open input file");
  if (fstat(in.io.fd, &st) != 0) {
    ibuf_close(&in);
    return respond_syserr(550, "Could not fstat input file");
  }
  if (!S_ISREG(st.st_mode)) {
    ibuf_close(&in);
    return respond(550, 1, "Requested name is not a regular file");
  }
  if (ss && !ibuf_seek(&in, ss)) {
    ibuf_close(&in);
    return respond(550, 1, "Could not seek to start position in input file.");
  }
  if (!make_out_connection(&out)) {
    ibuf_close(&in);
    return 1;
  }
  switch (copy_xlate_close(&in, &out, timeout * 1000,
			   binary_flag ? 0 : xlate_ascii,
			   &bytes_in, &bytes_out)) {
  case 0:
    return respond_bytes(226, "File sent successfully", bytes_out, 1);
  case 1:
    return respond_bytes(426, "File send timed out", bytes_out, 1);
  case 2:
    return respond_bytes(426, "File send interrupted", bytes_out, 1);
  default:
    return respond_bytes(451, "Sending file failed", bytes_out, 1);
  }
}
