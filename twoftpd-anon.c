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
  { "RETR", 0, 0,           handle_retr },
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
