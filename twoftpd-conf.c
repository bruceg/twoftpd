/* twoftpd-conf.c - Configure a new instance of twoftpd auth/xfer
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
#include "conf_bin.c"
#include "iobuf/iobuf.h"
#include "err/err.h"

const char program[] = "twoftpd-conf";
static const char usage_str[] =
"usage: twoftpd-conf logacct /twoftpd /twoftpd/log cvm [ip [chroot]]\n";

void usage(const char* msg)
{
  if (msg)
    obuf_put4s(&outbuf, program, ": ", msg, ".\n");
  obuf_puts(&outbuf, usage_str);
  obuf_flush(&outbuf);
  exit(1);
}

static struct passwd* logacct;
static const char* maindir;
static const char* logdir;
static const char* cvmpath;
static const char* ip = "0.0.0.0";
static const char* dochroot;

int main(int argc, char* argv[])
{
  if (argc < 5) usage("Too few parameters");
  if (argc > 7) usage("Too many parameters");
  logacct = getpwnam(argv[1]);
  maindir = argv[2];
  logdir = argv[3];
  if (maindir[0] != '/' || logdir[0] != '/')
    die1(1, "Directory names must start with /.");
  cvmpath = argv[4];
  if (argc > 5) {
    ip = argv[5];
    if (argc > 6) dochroot = argv[6];
  }
  if (!logacct) die1(1, "Unknown logacct user name");

  umask(0);

  if (mkdir(maindir, 0755) == -1) die1sys(1, "Error creating main directory");
  if (chmod(maindir, 01755) == -1)
    die1sys(1, "Error setting permissions on main directory");
  
  if (mkdir(logdir, 0700) == -1) die1sys(1, "Error creating log directory");
  if (chown(logdir, logacct->pw_uid, logacct->pw_gid) == -1)
    die1sys(1, "Error setting owner on log directory");

  if (chdir(maindir) == -1) die1sys(1, "Error changing to main directory");
  if (mkdir("log", 0755) == -1)
    die1sys(1, "Error creating log service directory");
  if (mkdir("env", 0755) == -1)
    die1sys(1, "Error creating env directory");

  start_file("run", 0755);
  obuf_put5s(&conf_out,
	     "#!/bin/sh\n"
	     "exec 2>&1\n"
	     "umask 022\n"
	     "exec \\\n"
	     "envdir ", maindir, "/env \\\n"
	     "tcpserver -DRHv -llocalhost ", ip, " 21 \\\n");
  obuf_put7s(&conf_out,
	     "softlimit -m 2000000 \\\n",
	     conf_bin, "/twoftpd-auth \\\n",
	     cvmpath, " \\\n",
	     conf_bin, "/twoftpd-xfer");
  end_file();

  make_file("log/run", 0755,
	    "#!/bin/sh\n"
	    "exec \\\n"
	    "setuidgid ", logacct->pw_name, " \\\n"
	    "multilog t ", logdir, 0, 0, 0);
  
  if (dochroot) make_fileu("env/CHROOT", 1);
  
  return 0;
}
