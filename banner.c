/* banner.c - Show a banner to the client.
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
#include <stdlib.h>
#include <string.h>
#include "twoftpd.h"

void show_banner(unsigned code, const char* banner) 
{
  char* copy;
  char* start;
  char* end;
  if ((copy = strdup(banner)) == 0) return;
  start = copy;
  while ((end = strchr(start, '\n')) != 0) {
    *end = 0;
    if (end > start) respond(code, 0, start);
    start = end + 1;
  }
  respond(code, 0, start);
  free(copy);
}
