/* twoftpd-drop.c - Main dispatch table for twoftpd-drop
 * Copyright (C) 2005  Bruce Guenter <bruceg@em.ca>
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

const command verbs[] = {
  { "TYPE", 0, 0,           handle_type },
  { "STRU", 0, 0,           handle_stru },
  { "MODE", 0, 0,           handle_mode },
  { "CWD",  0, 0,           handle_cwd },
  { "PWD",  0, handle_pwd,  0 },
  { "CDUP", 0, handle_cdup, 0 },
  { "PASV", 0, handle_pasv, 0 },
  { "PORT", 0, 0,           handle_port },
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
