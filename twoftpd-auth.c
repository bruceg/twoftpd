/* twoftpd-auth.c - Authentication front-end for twoftpd
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
#include <string.h>
#include <unistd.h>
#include "twoftpd.h"
#include "cvm/client.h"

static char** argv_anon = 0;
static char** argv_xfer = 0;

static int sent_user = 0;
static int anon = 0;
static const char* anon_name;
static const char* anon_home;
static uid_t anon_uid;
static gid_t anon_gid;
static const char* cvmodule = 0;
static const char* creds[3];

static char* utoa(unsigned i)
{
  static char buf[32];
  char* ptr;
  
  ptr = buf + sizeof buf - 1;

  *ptr-- = 0;
  if (!i)
    *ptr-- = '0';
  else {
    while (i) {
      *ptr-- = (i % 10) + '0';
      i /= 10;
    }
  }
  return strdup(ptr + 1);
}

static void do_exec(char** argv, int chroot, uid_t uid, gid_t gid,
		    const char* home, const char* user)
{
  if ((!chroot || !setenv("CHROOT", "1", 1)) &&
      !setenv("UID", utoa(uid), 1) &&
      !setenv("GID", utoa(gid), 1) &&
      !setenv("HOME", home, 1) &&
      !setenv("USER", user, 1))
    execvp(argv[0], argv);
  respond(421, 1, "Could not execute back-end.");
  exit(1);
}

static int handle_user(void)
{
  if (anon &&
      (!strcasecmp(req_param, "anonymous") ||
       !strcasecmp(req_param, "ftp")))
    do_exec(argv_anon, 1, anon_uid, anon_gid, anon_home, anon_name);
  if (creds[0]) free((char*)creds[0]);
  creds[0] = strdup(req_param);
  sent_user = 1;
  return respond(331, 1, "Send PASS.");
}

static int handle_pass(void)
{
  if (!sent_user) return respond(503, 1, "Send USER first.");
  sent_user = 0;
  creds[1] = req_param;
  creds[2] = 0;
  if (authenticate(cvmodule, creds) == 0)
    do_exec(argv_xfer, 0,
	    fact_userid, fact_groupid, fact_directory, fact_username);
  return respond(530, 1, "Authentication failed.");
}

verb verbs[] = {
  { "USER", 0, 0, handle_user },
  { "PASS", 1, 0, handle_pass },
  { 0,      0, 0, 0 }
};

int startup(int argc, char* argv[])
{
  int i;
  const char* tmp;
  const char* end;
  
  if (argc < 3) {
    respond(421, 1, "Configuration error, insufficient paramenters.");
    return 0;
  }
  
  cvmodule = argv[1];
  argv_xfer = argv + 2;
  for (i = 3; i < argc; ++i) {
    if (strcmp(argv[i], "--") == 0) {
      if (i >= argc - 1) {
	respond(421, 1, "Configuration error, missing anonymous program name.");
	return 0;
      }
      argv_anon = argv + i + 1;
      break;
    }
  }

  if (argv_anon) {
    anon_name = "nobody";
    if ((tmp = getenv("ANON_UID")) == 0 ||
	(anon_uid = strtou(tmp, &end)) == 0 || end == tmp || *end != 0 ||
	(tmp = getenv("ANON_GID")) == 0 ||
	(anon_gid = strtou(tmp, &end)) == 0 || end == tmp || *end != 0 ||
	(anon_home = getenv("ANON_HOME")) == 0) {
      respond(421, 1, "Configuration error, invalid anonymous configuration.");
      return 0;
    }
    anon = 1;
  }
  
  return respond(220, 0, "TwoFTPD server ready.") &&
    (!anon || respond(220, 0, "Anonymous login allowed.")) &&
    respond(220, 1, "Authenticate first.");
}
