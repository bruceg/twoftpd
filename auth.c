#include <crypt.h>
#include <pwd.h>
#include <shadow.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "twoftpd.h"

static char** exec_argv;

static int sent_user = 0;
static struct passwd* pw = 0;
static struct spwd* spw = 0;

static int verb_user(void)
{
  pw = getpwnam(req_param);
  spw = getspnam(req_param);
  sent_user = 1;
  return respond(331, 1, "Send PASS.");
}

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

static int do_exec(void)
{
  setenv("UID", utoa(pw->pw_uid), 1);
  setenv("GID", utoa(pw->pw_gid), 1);
  setenv("HOME", pw->pw_dir, 1);
  setenv("USER", pw->pw_name, 1);
  execvp(exec_argv[0], exec_argv);
  respond(421, 1, "Could not execute back-end.");
  return 0;
}

static int verb_pass(void)
{
  char* pwcrypt;
  
  if (!sent_user) return respond(503, 1, "Send USER first.");
  sent_user = 0;
  if (pw) {
    pwcrypt = spw ? spw->sp_pwdp : pw->pw_passwd;
    if (!strcmp(pwcrypt, crypt(req_param, pwcrypt))) {
      do_exec();
      return 0;
    }
  }
  return respond(530, 1, "Authentication failed.");
}

verb verbs[] = {
  { "USER", 0, verb_user },
  { "PASS", 0, verb_pass },
  { 0, 0, 0 }
};

int startup(int argc, char* argv[])
{
  exec_argv = argv + 1;
  return respond(220, 0, "Server ready.") &&
    respond(220, 1, "Authenticate first.");
}
