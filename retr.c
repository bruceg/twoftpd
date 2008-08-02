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
  int in;
  obuf out;
  unsigned long ss;
  struct stat st;
  unsigned long bytes_in;
  unsigned long bytes_out;
  int result;
  
  ss = startpos;
  startpos = 0;
  if ((in = open_fd(req_param, O_RDONLY, 0)) == -1)
    return respond_syserr(550, "Could not open input file");
  if (fstat(in, &st) != 0) {
    close(in);
    return respond_syserr(550, "Could not fstat input file");
  }
  if (!S_ISREG(st.st_mode)) {
    close(in);
    return respond(550, 1, "Requested name is not a regular file");
  }
  if (ss && !lseek(in, ss, SEEK_SET)) {
    close(in);
    return respond(550, 1, "Could not seek to start position in input file.");
  }
  if (!make_out_connection(&out)) {
    close(in);
    return 1;
  }
  result = copy_xlate_close(in, out.io.fd, timeout * 1000,
			    binary_flag ? 0 : xlate_ascii,
			    &bytes_in, &bytes_out);
  return respond_xferresult(result, bytes_out, 1);
}
