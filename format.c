#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <time.h>
#include "twoftpd.h"

static time_t now = 0;

static char* format_mode(int mode, char* buf)
{
  if (S_ISLNK(mode)) *buf = 'l';
  else if (S_ISREG(mode)) *buf = '-';
  else if (S_ISDIR(mode)) *buf = 'd';
  else if (S_ISCHR(mode)) *buf = 'c';
  else if (S_ISBLK(mode)) *buf = 'b';
  else if (S_ISFIFO(mode)) *buf = 'p';
  else if (S_ISSOCK(mode)) *buf = 's';
  else *buf = '?';
  buf++;
  *buf++ = (mode & S_IRUSR) ? 'r' : '-';
  *buf++ = (mode & S_IWUSR) ? 'w' : '-';
  *buf++ = (mode & S_ISUID) ?
    (mode & S_IXUSR) ? 's' : 'S' :
    (mode & S_IXUSR) ? 'x' : '-';
  *buf++ = (mode & S_IRGRP) ? 'r' : '-';
  *buf++ = (mode & S_IWGRP) ? 'w' : '-';
  *buf++ = (mode & S_ISGID) ?
    (mode & S_IXGRP) ? 's' : 'S' :
    (mode & S_IXGRP) ? 'x' : '-';
  *buf++ = (mode & S_IROTH) ? 'r' : '-';
  *buf++ = (mode & S_IWOTH) ? 'w' : '-';
  *buf++ = (mode & S_ISVTX) ?
    (mode & S_IXOTH) ? 't' : 'T' :
    (mode & S_IXOTH) ? 'x' : '-';
  return buf;
}

static char* format_num(unsigned num, unsigned digits, unsigned max,
			char* buf)
{
  unsigned i;
  if (!num) {
    *buf++ = '0';
    for (i = 1; i < digits; i++)
      *buf++ = ' ';
  }
  else if (num >= max) {
    for (i = 0; i < digits; i++)
      *buf++ = '9';
  }
  else {
    for (i = 0; i < digits; i++) {
      buf[digits-1-i] = num ? (num % 10) + '0' : SPACE;
      num /= 10;
    }
    buf += digits;
  }
  return buf;
}

static char* format_owner(uid_t uid, char* buf)
{
  static struct passwd* pw = 0;
  unsigned i;
  char* ptr;

  if (!pw || pw->pw_uid != uid) pw = getpwuid(uid);
  if (pw && pw->pw_name) {
    for (i = 0, ptr = pw->pw_name; *ptr && i < 8; ++i)
      *buf++ = *ptr++;
    for (; i < 8; ++i) *buf++ = SPACE;
  }
  else
    buf = format_num(uid, 8, 99999999, buf);
  return buf;
}

static char* format_group(gid_t gid, char* buf)
{
  static struct group* gr = 0;
  unsigned i;
  char* ptr;

  if (!gr || gr->gr_gid != gid) gr = getgrgid(gid);
  if (gr && gr->gr_name) {
    for (i = 0, ptr = gr->gr_name; *ptr && i < 8; ++i)
      *buf++ = *ptr++;
    for (; i < 8; ++i) *buf++ = SPACE;
  }
  else
    buf = format_num(gid, 8, 99999999, buf);
  return buf;
}

static char* format_time(time_t then, char* buf)
{
  struct tm* tm;
  time_t year;
  
  if (!now) now = time(0);
  year = now - 365/2*24*60*60;

  tm = localtime(&then);
  if (then > year)
    buf += strftime(buf, 13, "%b %e %H:%M", tm);
  else
    buf += strftime(buf, 13, "%b %e  %Y", tm);
  return buf;
}

void format_stat(const struct stat* s, const char* filename, char* buf)
{
  size_t namelen = strlen(filename);
  buf = format_mode(s->st_mode, buf); *buf++ = SPACE;
  buf = format_num(s->st_nlink, 4, 9999, buf); *buf++ = SPACE;
  buf = format_owner(s->st_uid, buf); *buf++ = SPACE;
  buf = format_group(s->st_gid, buf); *buf++ = SPACE;
  buf = format_num(s->st_size, 8, 99999999, buf); *buf++ = SPACE;
  buf = format_time(s->st_mtime, buf); *buf++ = SPACE;
  memcpy(buf, filename, namelen);
  buf += namelen;
  *buf++ = CR;
  *buf++ = LF;
  *buf = 0;
}
