#include <crypt.h>
#include <pwd.h>
#include <shadow.h>
#include <stdlib.h>
#include "twoftpd.h"
#include "frontend.h"

static struct passwd* pw = 0;
static struct spwd* spw = 0;

static authuser* translate(struct passwd* pw)
{
  authuser* au;
  au = malloc(sizeof(*au));
  if (!pw) return 0;
  au->uid = pw->pw_uid;
  au->gid = pw->pw_gid;
  au->home = pw->pw_dir;
  au->user = pw->pw_name;
  return au;
}

void auth_user(const char* username)
{
  pw = getpwnam(username);
  spw = getspnam(username);
}

authuser* auth_pass(const char* password)
{
  char* pwcrypt;

  if (pw) {
    pwcrypt = spw ? spw->sp_pwdp : pw->pw_passwd;
    if (!strcmp(pwcrypt, crypt(password, pwcrypt)))
      return translate(pw);
  }
  return 0;
}

authuser* auth_anon(const char* username)
{
  return translate(getpwnam(username));
}
