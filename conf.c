/* conf.c - Library for configuration programs
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
#include <pwd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "conf.h"
#include <iobuf/iobuf.h>
#include <msg/msg.h>

obuf conf_out;
static const char* currfile;

const int msg_show_pid = 0;

void start_file(const char* filename, int mode)
{
  currfile = filename;
  if (!obuf_open(&conf_out, filename, OBUF_CREATE|OBUF_EXCLUSIVE, mode, 0))
    die3sys(1, "Error creating file '", currfile, "'");
}

void end_file(void)
{
  if (!obuf_endl(&conf_out) || !obuf_close(&conf_out))
    die3sys(1, "Error creating file '", currfile, "'");
}

void make_file(const char* filename, int mode,
	       const char* s1,
	       const char* s2,
	       const char* s3,
	       const char* s4,
	       const char* s5,
	       const char* s6,
	       const char* s7)
{
  start_file(filename, mode);
  obuf_put7s(&conf_out, s1, s2, s3, s4, s5, s6, s7);
  end_file();
}

void make_fileu(const char* filename, unsigned val)
{
  start_file(filename, 0644);
  obuf_putu(&conf_out, val);
  end_file();
}
