#ifndef TWOFTPD__BACKEND__H__
#define TWOFTPD__BACKEND__H__

#include <sys/stat.h>
#include <sys/types.h>

extern time_t now;

/* In backend.c */
extern const char* tcplocalip;
extern int handle_pass(void);

/* In listdir.c */
extern const char** listdir(const char* path);

/* In list.c */
extern int handle_list(void);

/* In nlst.c */
extern int handle_nlst(void);

/* In retr.c */
extern int handle_retr(void);

/* In socket.c */
extern int make_connection(void);
extern int handle_pasv(void);
extern int handle_port(void);

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

#endif
