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

const char program[] = "twoftpd-anon-conf";
static const char usage_str[] =
"usage: twoftpd-anon-conf ftpacct logacct /twoftpd /twoftpd/log /ftp [ip]\n";

void usage(const char* msg)
{
  if (msg)
    obuf_put4s(&outbuf, program, ": ", msg, ".\n");
  obuf_puts(&outbuf, usage_str);
  obuf_flush(&outbuf);
  exit(1);
}

static struct passwd* ftpacct;
static struct passwd* logacct;
static const char* maindir;
static const char* logdir;
static const char* ftpdir;
static const char* ip = "0.0.0.0";

int main(int argc, char* argv[])
{
  if (argc < 7) usage("Too few parameters");
  if (argc > 8) usage("Too many parameters");
  ftpacct = getpwnam(argv[1]);
  logacct = getpwnam(argv[2]);
  maindir = argv[3];
  logdir = argv[4];
  ftpdir = argv[5];
  if (maindir[0] != '/' || logdir[0] != '/' || ftpdir[0] != '/')
    die1(1, "Directory names must start with /.");
  if (argc > 7) {
    ip = argv[7];
  }
  if (!ftpacct) die1(1, "Unknown ftpacct user name");
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

  make_file("run", 0755,
	    "#!/bin/sh\n"
	    "exec 2>&1\n"
	    "umask 022\n"
	    "exec \\\n"
	    "envdir ", maindir, "/env \\\n"
	    "tcpserver -DRHv -llocalhost ", ip, " 21 \\\n"
	    "softlimit -m 2000000 \\\n",
	    conf_bin, "twoftpd-anon");
  make_file("log/run", 0755,
	    "#!/bin/sh\n"
	    "exec \\\n"
	    "setuidgid ", logacct->pw_name, " \\\n"
	    "multilog t ", logdir, 0, 0, 0);
  make_fileu("env/CHROOT", 1);
  make_fileu("env/GID", ftpacct->pw_gid);
  make_file("env/HOME", 0644, ftpdir, 0, 0, 0, 0, 0, 0);
  make_fileu("env/UID", ftpacct->pw_uid);
  make_file("env/USER", 0644, "ftp", 0, 0, 0, 0, 0, 0);
  
  return 0;
}
