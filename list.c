#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

static char* format_mode(int mode, char* buf)
{
  memcpy(buf, S_ISDIR(mode) ? "drwxr-xr-x" :
	 S_ISREG(mode) ? "-rw-r--r--" : "??????????", 1);
  return buf + 10;
}

static char* format_owner(uid_t owner, char* buf)
{
  unsigned i;
  if (owner == uid) {
    memcpy(buf, user, user_len > 8 ? 8 : user_len);
    for (i = user_len; i < 8; i++)
      buf[i] = SPACE;
  }
  else
    memcpy(buf, "somebody", 8);
  return buf + 8;
}

static char* format_group(gid_t group, char* buf)
{
  memcpy(buf, (group == gid) ? "group   " : "somegrp ", 8);
  return buf + 8;
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

static char* format_time(time_t then, char* buf)
{
  struct tm* tm;
  time_t year;
  
  year = now - 365/2*24*60*60;

  tm = localtime(&then);
  if (then > year)
    buf += strftime(buf, 13, "%b %e %H:%M", tm);
  else
    buf += strftime(buf, 13, "%b %e  %Y", tm);
  return buf;
}

static void format_stat(const struct stat* s, const char* filename,
			char* buf, unsigned maxlen)
{
  char* start;
  size_t namelen;
  
  start = buf;
  namelen = strlen(filename);
  buf = format_mode(s->st_mode, buf); *buf++ = SPACE;
  memcpy(buf, "    1 ", 6); buf += 6;
  buf = format_owner(s->st_uid, buf); *buf++ = SPACE;
  buf = format_group(s->st_gid, buf); *buf++ = SPACE;
  buf = format_num(s->st_size, 8, 99999999, buf); *buf++ = SPACE;
  buf = format_time(s->st_mtime, buf); *buf++ = SPACE;

  if (namelen >= maxlen-(buf-start)-1) namelen = maxlen-(buf-start)-1;
  
  memcpy(buf, filename, namelen);
  buf += namelen;
  *buf++ = CR;
  *buf++ = LF;
  *buf = 0;
}

static int pushd(const char* path)
{
  int cwd;
  if ((cwd = open(".", O_RDONLY)) == -1) return -1;
  if (chdir(path) == -1) {
    close(cwd);
    return -1;
  }
  return cwd;
}

int handle_list(void)
{
  int fd;
  int cwd;
  const char** entries;
  struct stat statbuf;
  char buffer[BUFSIZE];
  
  if (req_param) {
    if ((cwd = pushd(req_param)) == -1)
      return respond(550, 1, "Could not list directory.");
  }
  else
    cwd = -1;
  if ((entries = listdir(".")) == 0) {
    fchdir(cwd);
    return respond(550, 1, "Could not list directory.");
  }

  if ((fd = make_connection()) == -1) return 1;

  while (*entries) {
    if (stat(*entries, &statbuf) != -1) {
      format_stat(&statbuf, *entries, buffer, sizeof buffer);
      write(fd, buffer, strlen(buffer));
    }
    ++entries;
  }
  close(fd);
  fchdir(cwd);
  return respond(226, 1, "Transfer complete.");
}
