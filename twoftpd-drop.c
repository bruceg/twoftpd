/* twoftpd-drop.c - Main dispatch table for twoftpd-drop
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
#include "twoftpd.h"
#include "backend.h"

const char program[] = "twoftpd-drop";

int store_exclusive = 1;

/* Most GUI FTP clients have problems if they can't list directories
 * before uploading, so we need to produce a fake empty listing for
 * their sake. */
static int fake_list(void)
{
  obuf out;
  if (!make_out_connection(&out)) return 1;
  if (!close_out_connection(&out))
    return respond(426, 1, "Listing aborted");
  return respond(226, 1, "Listing complete");
}

const command verbs[] = {
  { "TYPE", 0, 0,           handle_type },
  { "STRU", 0, 0,           handle_stru },
  { "MODE", 0, 0,           handle_mode },
  { "CWD",  0, 0,           handle_cwd },
  { "PWD",  0, handle_pwd,  0 },
  { "CDUP", 0, handle_cdup, 0 },
  { "PASV", 0, handle_pasv, 0 },
  { "PORT", 0, 0,           handle_port },
  { "LIST", 0, fake_list,   fake_list },
  { "NLST", 0, fake_list,   fake_list },
  { "STOR", 0, 0,           handle_stor },
  /* Compatibility verbs as defined by RFC1123 */
  { "XCWD", 0, 0,           handle_cwd },
  { "XPWD", 0, handle_pwd,  0 },
  { "XCUP", 0, handle_cdup, 0 },
  /* Handle stray login commands */
  { "USER", 1, 0,           handle_pass },
  { "PASS", 1, 0,           handle_pass },
  { "ACCT", 0, 0,           handle_pass },
  { 0,      0, 0,           0 }
};

const command site_commands[] = {
  { 0,       0, 0, 0 }
};
