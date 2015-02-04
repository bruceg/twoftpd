/* twoftpd-switch.c - Switch between the three twoftpd back-ends.
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
#include <bglibs/sysdeps.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "twoftpd.h"
#include "conf_bin.c"

const char program[] = "twoftpd-switch";
const int msg_show_pid = 1;

static void exec(const char* name)
{
  char cmd[sizeof conf_bin + 13];
  char* argv[2] = { cmd, 0 };
  strcpy(cmd, conf_bin);
  strcpy(cmd + sizeof conf_bin - 1, "/twoftpd-");
  strcpy(cmd + sizeof conf_bin + 8, name);
  execv(cmd, argv);
  respond_syserr(421, "Could not exec backend");
  _exit(111);
}

int main(void)
{
  const char *shell;
  const char *env;
  if ((shell = getenv("SHELL")) != 0) {
    if ((env = getenv("SHELL_WRITEONLY")) != 0
	&& strcmp(shell, env) == 0)
      exec("drop");
    if ((env = getenv("SHELL_READONLY")) != 0
	&& strcmp(shell, env) == 0)
      exec("anon");
  }
  exec("xfer");
  return 111;
}
