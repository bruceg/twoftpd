#include "twoftpd.h"

extern int handle_pass(void);

verb verbs[] = {
  { "PASS", 1, 0,           handle_pass },
  { "ACCT", 0, 0,           handle_pass },
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
  { "STOR", 0, 0,           handle_stor },
  { "APPE", 0, 0,           handle_appe },
  { "MKD",  0, 0,           handle_mkd  },
  { "RMD",  0, 0,           handle_rmd  },
  { "DELE", 0, 0,           handle_dele },
  /* Compatibility verbs as defined by RFC1123 */
  { "XCWD", 0, 0,           handle_cwd },
  { "XPWD", 0, handle_pwd,  0 },
  { "XCUP", 0, handle_cdup, 0 },
  { "XMKD", 0, 0,           handle_mkd  },
  { "XRMD", 0, 0,           handle_rmd  },
  { 0,      0, 0,           0 }
};
