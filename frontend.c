#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "twoftpd.h"

static char** argv_anon = 0;
static char** argv_xfer = 0;

static int sent_user = 0;
static authuser* anonau = 0;

static char* utoa(unsigned i)
{
  char* buf = malloc(16);
  char* ptr = buf + 15;
  *ptr-- = 0;
  if (!i)
    *ptr-- = '0';
  else {
    while (i) {
      *ptr-- = (i % 10) + '0';
      i /= 10;
    }
  }
  return ptr + 1;
}

static void do_exec(authuser* au, char** argv, int chroot)
{
  if ((!chroot || !setenv("CHROOT", "1", 1)) &&
      !setenv("UID", utoa(au->uid), 1) &&
      !setenv("GID", utoa(au->gid), 1) &&
      !setenv("HOME", au->home, 1))
    execvp(argv[0], argv);
  respond(421, 1, "Could not execute back-end.");
  exit(1);
}

static int handle_user(void)
{
  if (anonau &&
      (!strcasecmp(req_param, "anonymous") ||
       !strcasecmp(req_param, "ftp")))
    do_exec(anonau, argv_anon, 1);
  auth_user(req_param);
  sent_user = 1;
  return respond(331, 1, "Send PASS.");
}

static int handle_pass(void)
{
  authuser* au;
  if (!sent_user) return respond(503, 1, "Send USER first.");
  sent_user = 0;
  if ((au = auth_pass(req_param)) != 0)
    do_exec(au, argv_xfer, 0);
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
  
  argv_xfer = argv + 1;
  for (i = 2; i < argc - 1; ++i) {
    if (strcmp(argv[i], "--") == 0) {
      argv_anon = argv + i + 1;
      break;
    }
  }
  if (!argv_anon) {
    respond(421, 1, "Configuration error, no program to execute.");
    return 0;
  }
  
  if ((tmp = getenv("ANONYMOUS")) != 0) {
    if ((anonau = auth_anon(tmp)) == 0) {
      respond(421, 1, "Configuration error, unknown anonymous user name.");
      return 0;
    }
  }
  
  return respond(220, 0, "TwoFTPD server ready.") &&
    (!anonau || respond(220, 0, "Anonymous login allowed.")) &&
    respond(220, 1, "Authenticate first.");
}
