/* backend.c - twoftpd back-end startup code
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
#include "systime.h"
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

uid_t uid;
gid_t gid;
const char* home;
const char* user;
unsigned user_len;
const char* group;
unsigned group_len;
time_t now;
int lockhome;
int nodotfiles;

int handle_pass(void)
{
  return respond(230, 1, "Access has already been granted.");
}

static int load_tables(void)
{
  /* Load some initial values that cannot be loaded once the chroot is done. */
  
  /* A call to localtime should load and cache the timezone setting. */
  now = time(0);
  localtime(&now);
  return 1;
}

#define FAIL(MSG) do{ respond(421, 1, MSG); return 0; }while(0)

int startup(int argc, char* argv[])
{
  const char* tmp;
  const char* end;
  const char* cwdstr;
  unsigned long session_timeout;
  
  if ((tmp = getenv("TCPLOCALIP")) == 0) FAIL("Missing $TCPLOCALIP.");
  if (!parse_localip(tmp)) FAIL("Could not parse $TCPLOCALIP.");
  if ((tmp = getenv("TCPREMOTEIP")) == 0) FAIL("Missing $TCPREMOTEIP.");
  if (!parse_remoteip(tmp)) FAIL("Could not parse $TCPREMOTEIP.");
  if ((tmp = getenv("UID")) == 0) FAIL("Missing $UID.");
  if (!(uid = strtou(tmp, &end)) || *end) FAIL("Invalid $UID.");
  if ((tmp = getenv("GID")) == 0) FAIL("Missing $GID.");
  if (!(gid = strtou(tmp, &end)) || *end) FAIL("Invalid $GID.");
  if ((home = getenv("HOME")) == 0) FAIL("Missing $HOME.");
  if ((user = getenv("USER")) == 0) FAIL("Missing $USER.");
  if ((group = getenv("GROUP")) == 0) group = "mygroup";
  if (chdir(home)) FAIL("Could not chdir to $HOME.");
  if (!load_tables()) FAIL("Loading startup tables failed.");
  if (getenv("CHROOT") != 0) {
    cwdstr = "/";
    if (chroot(".")) FAIL("Could not chroot.");
  }
  else if (getenv("SOFTCHROOT") != 0) {
    cwdstr = "/";
  }
  else {
    cwdstr = home;
    if (chdir("/")) FAIL("Could not chdir to '/'.");
  }
  if (!str_copys(&cwd, cwdstr)) FAIL("Could not set CWD string");
  if (setgid(gid)) FAIL("Could not set GID.");
  if (setuid(uid)) FAIL("Could not set UID.");
  user_len = strlen(user);
  group_len = strlen(group);

  lockhome = (getenv("LOCKHOME") != 0);
  nodotfiles = (getenv("NODOTFILES") != 0);

  session_timeout = 0;
  if ((tmp = getenv("SESSION_TIMEOUT")) != 0)
    session_timeout = strtou(tmp, &tmp);
  alarm(session_timeout);

  if ((tmp = getenv("BANNER")) != 0) show_banner(startup_code, tmp);
  message_file = getenv("MESSAGEFILE");
  show_message_file(startup_code);
  return respond(startup_code, 1, "Ready to transfer files.");
  (void)argc;
  (void)argv;
}
