#include <crypt.h>
#include <pwd.h>
#include <stdlib.h>
#include "hasspnam.h"
#include "twoftpd.h"
#include "frontend.h"

static struct passwd* pw = 0;

#ifdef HASGETSPNAM
#include <shadow.h>
static struct spwd* spw = 0;
#endif

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
#ifdef HASGETSPNAM
  spw = getspnam(username);
#endif
}

authuser* auth_pass(const char* password)
{
  char* pwcrypt;

  if (pw) {
#ifdef HASGETSPNAM
    pwcrypt = spw ? spw->sp_pwdp : pw->pw_passwd;
#else
    pwcrypt = pw->pw_passwd;
#endif
    if (!strcmp(pwcrypt, crypt(password, pwcrypt)))
      return translate(pw);
  }
  return 0;
}
