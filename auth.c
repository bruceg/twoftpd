#include <crypt.h>
#include <pwd.h>
#include <shadow.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "twoftpd.h"

static char** argv_anon = 0;
static char** argv_xfer = 0;

static const char* anon_home;
static const char* anon_uid;
static const char* anon_gid;
static int allow_anon;

static int sent_user = 0;
static struct passwd* pw = 0;
static struct spwd* spw = 0;

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

static int exec_anon(void)
{
  if (!setenv("CHROOT", "1", 1) &&
      !setenv("UID", anon_uid, 1) &&
      !setenv("GID", anon_gid, 1) &&
      !setenv("HOME", anon_home, 1))
    execvp(argv_anon[0], argv_anon);
  respond(421, 1, "Could not execute back-end.");
  exit(1);
}

static int exec_xfer(void)
{
  if (!setenv("UID", utoa(pw->pw_uid), 1) &&
      !setenv("GID", utoa(pw->pw_gid), 1) &&
      !setenv("HOME", pw->pw_dir, 1))
    execvp(argv_xfer[0], argv_xfer);
  respond(421, 1, "Could not execute back-end.");
  exit(1);
}

static int verb_user(void)
{
  if (allow_anon &&
      (!strcasecmp(req_param, "anonymous") ||
       !strcasecmp(req_param, "ftp")))
    exec_anon();
  pw = getpwnam(req_param);
  spw = getspnam(req_param);
  sent_user = 1;
  return respond(331, 1, "Send PASS.");
}

static int verb_pass(void)
{
  char* pwcrypt;

  if (!sent_user) return respond(503, 1, "Send USER first.");
  sent_user = 0;
  if (pw) {
    pwcrypt = spw ? spw->sp_pwdp : pw->pw_passwd;
    if (!strcmp(pwcrypt, crypt(req_param, pwcrypt))) {
      exec_xfer();
      return 0;
    }
  }
  return respond(530, 1, "Authentication failed.");
}

verb verbs[] = {
  { "USER", 0, 0, verb_user },
  { "PASS", 1, 0, verb_pass },
  { 0,      0, 0, 0 }
};

int startup(int argc, char* argv[])
{
  int i;

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
    
  anon_home = getenv("ANON_HOME");
  anon_gid = getenv("ANON_GID");
  anon_uid = getenv("ANON_UID");
  allow_anon = anon_home && anon_gid && anon_uid;
  
  return respond(220, 0, "TwoFTPD server ready.") &&
    (!allow_anon || respond(220, 0, "Anonymous login allowed.")) &&
    respond(220, 1, "Authenticate first.");
}
