#ifndef TWOFTPD__FRONTEND__H__
#define TWOFTPD__FRONTEND__H__

#include <sys/stat.h>
#include <sys/types.h>

struct authuser
{
  uid_t uid;
  gid_t gid;
  const char* home;
};
typedef struct authuser authuser;

/* In auth.c */
extern void auth_user(const char*);
extern authuser* auth_pass(const char*);
extern authuser* auth_anon(const char*);

#endif
