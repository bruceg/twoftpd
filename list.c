#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

static obuf out;

static int output_mode(int mode)
{
  return obuf_write(&out, S_ISDIR(mode) ? "drwxr-xr-x" :
		    S_ISREG(mode) ? "-rw-r--r--" : "??????????", 10);
}

static int output_owner(uid_t owner)
{
  unsigned i;
  if (owner == uid) {
    if (!obuf_write(&out, user, user_len > 8 ? 8 : user_len)) return 0;
    for (i = user_len; i < 8; i++)
      if (!obuf_putc(&out, SPACE)) return 0;
  }
  else
    if (!obuf_write(&out, "somebody", 8)) return 0;
  return 1;
}

static int output_group(gid_t group)
{
  return obuf_write(&out, (group == gid) ? "group   " : "somegrp ", 8);
}

static int output_time(time_t then)
{
  struct tm* tm;
  time_t year;
  char buf[16];
  unsigned len;
  
  year = now - 365/2*24*60*60;

  tm = localtime(&then);
  if (then > year)
    len = strftime(buf, sizeof buf - 1, "%b %e %H:%M", tm);
  else
    len = strftime(buf, sizeof buf - 1, "%b %e  %Y", tm);
  return obuf_write(&out, buf, len);
}

static int output_stat(const char* filename, const struct stat* s)
{
  return output_mode(s->st_mode) &&
    obuf_puts(&out, "    1 ") &&
    output_owner(s->st_uid) &&
    obuf_putc(&out, SPACE) &&
    output_group(s->st_gid) &&
    obuf_putc(&out, SPACE) &&
    obuf_putuw(&out, s->st_size, 8) &&
    obuf_putc(&out, SPACE) &&
    output_time(s->st_mtime) &&
    obuf_putc(&out, SPACE) &&
    obuf_puts(&out, filename) &&
    obuf_puts(&out, "\r\n");
}

static int output_line(const char* name)
{
  return obuf_puts(&out, name) && obuf_puts(&out, "\r\n");
}

static int list_single(const char* name)
{
  if (!make_out_connection(&out)) return 1;
  if (!output_line(name) || !obuf_flush(&out)) {
    obuf_close(&out);
    return respond(426, 1, "Transfer aborted.");
  }
  obuf_close(&out);
  return respond(226, 1, "Transfer complete.");
}

static int list_single_long(const char* name, const struct stat* stat)
{
  if (!make_out_connection(&out)) return 1;
  if (!output_stat(name, stat)) {
    obuf_close(&out);
    return respond(426, 1, "Transfer aborted.");
  }
  obuf_close(&out);
  return respond(226, 1, "Transfer complete.");
}

static int list_directory(int longfmt)
{
  const char** entries;
  struct stat statbuf;
  int result;

  entries = listdir(".");
  if (entries == 0)
    return respond(550, 1, "Could not list directory.");

  if (!make_out_connection(&out)) return 1;

  while (*entries) {
    if (longfmt) {
      if (stat(*entries, &statbuf) == -1) {
	obuf_close(&out);
	return respond(451, 1, "Error reading directory.");
      }
      result = output_stat(*entries, &statbuf);
    }
    else
      result = output_line(*entries);
    if (!result) {
      obuf_close(&out);
      return respond(426, 1, "Transfer aborted.");
    }
    ++entries;
  }
  if (!obuf_flush(&out)) {
    obuf_close(&out);
    return respond(426, 1, "Transfer aborted.");
  }
  obuf_close(&out);
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
