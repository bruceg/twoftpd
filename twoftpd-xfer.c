/* twoftpd-xfer.c - Main dispatch table for twoftpd-xfer
 * Copyright (C) 2001  Bruce Guenter <bruceg@em.ca>
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

verb verbs[] = {
  { "TYPE", 0, 0,           handle_type },
  { "STRU", 0, 0,           handle_stru },
  { "MODE", 0, 0,           handle_mode },
  { "CWD",  0, 0,           handle_cwd },
  { "PWD",  0, handle_pwd,  0 },
  { "CDUP", 0, handle_cdup, 0 },
  { "PASV", 0, handle_pasv, 0 },
  { "PORT", 0, 0,           handle_port },
  { "LIST", 0, handle_list, handle_list },
  { "NLST", 0, handle_nlst, handle_nlst },
  { "SIZE", 0, 0,           handle_size },
  { "MDTM", 0, 0,           handle_mdtm },
  { "REST", 0, 0,           handle_rest },
  { "RETR", 0, 0,           handle_retr },
  { "STOR", 0, 0,           handle_stor },
  { "APPE", 0, 0,           handle_appe },
  { "MKD",  0, 0,           handle_mkd  },
  { "RMD",  0, 0,           handle_rmd  },
  { "DELE", 0, 0,           handle_dele },
  { "RNFR", 0, 0,           handle_rnfr },
  { "RNTO", 0, 0,           handle_rnto },
  /* Compatibility verbs as defined by RFC1123 */
  { "XCWD", 0, 0,           handle_cwd },
  { "XPWD", 0, handle_pwd,  0 },
  { "XCUP", 0, handle_cdup, 0 },
  { "XMKD", 0, 0,           handle_mkd  },
  { "XRMD", 0, 0,           handle_rmd  },
  /* Handle stray login commands */
  { "USER", 1, 0,           handle_pass },
  { "PASS", 1, 0,           handle_pass },
  { "ACCT", 0, 0,           handle_pass },
  { 0,      0, 0,           0 }
};
