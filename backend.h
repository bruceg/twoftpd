#ifndef TWOFTPD__BACKEND__H__
#define TWOFTPD__BACKEND__H__

#include "iobuf/iobuf.h"
#include "str/str.h"
#include <sys/stat.h>
#include <sys/types.h>

extern time_t now;
extern const char* user;
extern const char* home;
extern unsigned user_len;
extern uid_t uid;
extern gid_t gid;
extern str cwd;
extern int lockhome;
extern int nodotfiles;

extern const unsigned startup_code;

/* In backend.c */
extern void show_message_file(unsigned code);
extern int handle_pass(void);

/* In list.c */
extern int handle_list(void);
extern int handle_nlst(void);

/* In messagefile.c */
extern const char* message_file;
extern void show_message_file(unsigned code);

/* In path.c */
extern str fullpath;
extern int qualify_validate(const char* path);
extern int open_in(ibuf* in, const char* filename);
extern int open_out(obuf* out, const char* filename, int flags);

/* In retr.c */
extern int handle_rest(void);
extern int handle_retr(void);

/* In socket.c */
extern int handle_pasv(void);
extern int handle_port(void);
extern int make_in_connection(ibuf*);
extern int make_out_connection(obuf*);
extern int parse_localip(const char*);
extern int parse_remoteip(const char*);

/* In stat.c */
extern int handle_size(void);
extern int handle_mdtm(void);

/* In state.c */
extern int binary_flag;
extern int handle_type(void);
extern int handle_stru(void);
extern int handle_mode(void);
extern int handle_cwd(void);
extern int handle_pwd(void);
extern int handle_cdup(void);

/* In store.c */
extern int handle_stor(void);
extern int handle_appe(void);
extern int handle_mkd(void);
extern int handle_rmd(void);
extern int handle_dele(void);
extern int handle_rnfr(void);
extern int handle_rnto(void);
extern int handle_site_chmod(void);

#endif
