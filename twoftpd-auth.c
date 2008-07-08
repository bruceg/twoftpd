/* twoftpd-auth.c - Authentication front-end for twoftpd
 * Copyright (C) 2008  Bruce Guenter <bruceg@em.ca>
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

#include <sysdeps.h>
#include <cvm/v2client.h>

#include "twoftpd.h"

const char program[] = "twoftpd-auth";

static char** argv_xfer = 0;

static const char* user = 0;
static const char* cvmodule = 0;
static unsigned long auth_attempts;

static void do_exec()
{
  alarm(0);
  if (putenv("AUTHENTICATED=1") == 0
      && cvm_setenv())
    execvp(argv_xfer[0], argv_xfer);
  respond(421, 1, "Could not execute back-end.");
  exit(1);
}

static int handle_user(void)
{
  if (user) free((char*)user);
  user = strdup(req_param);
  return respond(331, 1, "Send PASS.");
}

static int handle_pass(void)
{
  if (!user) return respond(503, 1, "Send USER first.");
  if (cvm_authenticate_password(cvmodule, user, getenv("TCPLOCALHOST"),
				req_param, 0) == 0)
    do_exec();
  free((char*)user);
  user = 0;
  if (!respond(530, 1, "Authentication failed."))
    return 0;
  if (auth_attempts-- <= 1) {
    respond(421, 1, "Too many authentication failures.");
    return 0;
  }
  return 1;
}

const command verbs[] = {
  { "USER", 0, 0, handle_user },
  { "PASS", 1, 0, handle_pass },
  { 0,      0, 0, 0 }
};

const command site_commands[] = {
  { 0,      0, 0, 0 }
};

int startup(int argc, char* argv[])
{
  const char* tmp;
  unsigned long auth_timeout;
  
  if (argc < 3) {
    respond(421, 1, "Configuration error, insufficient paramenters.");
    return 0;
  }
  
  cvmodule = argv[1];
  argv_xfer = argv + 2;

  if (!getenv("SERVICE") && putenv("SERVICE=ftp") == -1) {
    respond(421, 1, "Error setting $SERVICE.");
    return 0;
  }

  if ((tmp = getenv("LOGINBANNER")) != 0) show_banner(220, tmp);

  auth_timeout = 0;
  if ((tmp = getenv("AUTH_TIMEOUT")) != 0) auth_timeout = strtou(tmp, &tmp);
  alarm(auth_timeout);

  auth_attempts = ~0UL;
  if ((tmp = getenv("AUTH_ATTEMPTS")) != 0)
    auth_attempts = strtou(tmp, &tmp);

  return respond(220, 0, "TwoFTPd server ready.") &&
    respond(220, 1, "Authenticate first.");
}
