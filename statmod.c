/* statmod.c - twoftpd routines for modifying file status
 * Copyright (C) 2008  Bruce Guenter <bruce@untroubled.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <utime.h>
#include <bglibs/systime.h>
#include "twoftpd.h"
#include "backend.h"

static int getint(const char* s, unsigned digits)
{
  int i;
  for (i = 0; digits > 0; ++s, --digits)
    i = (i * 10) + *s - '0';
  return i;
}

static int gettime(const char* s, char end, time_t* stamp)
{
  int i;
  struct tm tm;

  for (i = 0; i < 14; ++i)
    if (!isdigit(s[i]))
      return 0;
  if (s[14] != end)
    return 0;

  tm.tm_year = getint(req_param+0,  4) - 1900;
  tm.tm_mon  = getint(req_param+4,  2) - 1;
  tm.tm_mday = getint(req_param+6,  2);
  tm.tm_hour = getint(req_param+8,  2);
  tm.tm_min  = getint(req_param+10, 2);
  tm.tm_sec  = getint(req_param+12, 2);
  tm.tm_wday = tm.tm_yday = tm.tm_isdst = 0;
  if (tm.tm_year < 70 ||
      tm.tm_mon < 0 || tm.tm_mon >= 12 ||
      tm.tm_mday < 1 || tm.tm_mday > 31 ||
      tm.tm_hour < 0 || tm.tm_hour >= 24 ||
      tm.tm_min < 0 || tm.tm_min >= 60 ||
      tm.tm_sec < 0 || tm.tm_sec > 61 ||
      (*stamp = mktime(&tm)) == -1)
    return -1;
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  *stamp -= tm.tm_gmtoff;
#else
  *stamp -= timezone;
#endif
  return 1;
}

int handle_mdtm2(void)
{
  time_t stamp;
  struct utimbuf ut;

  switch (gettime(req_param, ' ', &stamp)) {
  case 0: return handle_mdtm();
  case -1: return respond(501, 1, "Invalid timestamp");
  }

  req_param += 15;
  if (!qualify_validate(req_param)) return 1;
  ut.actime = ut.modtime = stamp;
  if (utime(fullpath.s+1, &ut) == -1)
    return respond_syserr(550, "Could not set file time");
  return respond(200, 1, "File time set");
}
