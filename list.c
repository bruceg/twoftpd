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
	 S_ISREG(mode) ? "-rw-r--r--" : "??????????", 10);
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

static int output_str(int fd, const char* str)
{
  size_t len;
  len = strlen(str);
  return write(fd, str, len) == len;
}

static int output_stat(int fd, const char* name, const struct stat* stat)
{
  char buffer[4096];
  format_stat(stat, name, buffer, sizeof buffer);
  return output_str(fd, buffer);
}

static int output_line(int fd, const char* name)
{
  return output_str(fd, name) && output_str(fd, "\r\n");
}

static int list_single(const char* name)
{
  int fd;
  
  if ((fd = make_connection()) == -1) return 1;
  if (!output_line(fd, name)) {
    close(fd);
    return respond(426, 1, "Transfer aborted.");
  }
  close(fd);
  return respond(226, 1, "Transfer complete.");
}

static int list_single_long(const char* name, const struct stat* stat)
{
  int fd;
  
  if ((fd = make_connection()) == -1) return 1;
  if (!output_stat(fd, name, stat)) {
    close(fd);
    return respond(426, 1, "Transfer aborted.");
  }
  close(fd);
  return respond(226, 1, "Transfer complete.");
}

static int list_directory(int longfmt)
{
  const char** entries;
  int fd;
  struct stat statbuf;
  int result;

  entries = listdir(".");
  if (entries == 0)
    return respond(550, 1, "Could not list directory.");

  if ((fd = make_connection()) == -1) return 1;

  while (*entries) {
    if (longfmt) {
      if (stat(*entries, &statbuf) == -1) {
	close(fd);
	return respond(451, 1, "Error reading directory.");
      }
      result = output_stat(fd, *entries, &statbuf);
    }
    else
      result = output_line(fd, *entries);
    if (!result) {
      close(fd);
      return respond(426, 1, "Transfer aborted.");
    }
    ++entries;
  }
  close(fd);
  return respond(226, 1, "Transfer complete.");
}

int handle_listing(int longfmt)
{
  int cwd;
  int result;
  struct stat statbuf;
  
  cwd = -1;
  if (req_param) {
    if (stat(req_param, &statbuf) == -1)
      return respond(550, 1, "File or directory does not exist.");
    if (!S_ISDIR(statbuf.st_mode))
      return longfmt ? 
	list_single_long(req_param, &statbuf) : list_single(req_param);
    if ((cwd = open(".", O_RDONLY)) == -1 ||
	chdir(req_param) == -1) {
      close(cwd);
      return respond(550, 1, "Could not list directory.");
    }
  }

  result = list_directory(longfmt);
  if (cwd != -1) {
    fchdir(cwd);
    close(cwd);
  }
  return result;
}

int handle_list(void)
{
  return handle_listing(1);
}

int handle_nlst(void)
{
  return handle_listing(0);
}
