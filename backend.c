#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

int do_chroot;
const char* tcplocalip;
uid_t uid;
gid_t gid;
const char* home;
const char* user;
unsigned user_len;
time_t now;

int handle_pass(void)
{
  return respond(202, 1, "Access has already been granted.");
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
  char* tmp;

  do_chroot = !!getenv("CHROOT");

  if ((tcplocalip = getenv("TCPLOCALIP")) == 0) FAIL("Missing $TCPLOCALIP.");
  if ((tmp = getenv("UID")) == 0) FAIL("Missing $UID.");
  if ((uid = atoi(tmp)) <= 0) FAIL("Invalid $UID.");
  if ((tmp = getenv("GID")) == 0) FAIL("Missing $GID.");
  if ((gid = atoi(tmp)) <= 0) FAIL("Invalid $GID.");
  if ((home = getenv("HOME")) == 0) FAIL("Missing $HOME.");
  if ((user = getenv("USER")) == 0) FAIL("Missing $USER.");
  if (chdir(home)) FAIL("Could not chdir to $HOME.");
  if (!load_tables()) FAIL("Loading startup tables failed.");
  if (do_chroot) if (chroot(".")) FAIL("Could not chroot.");
  if (setgid(gid)) FAIL("Could not set GID.");
  if (setuid(uid)) FAIL("Could not set UID.");
  user_len = strlen(user);
  return respond(202, 1, "Ready to transfer files.");
}
