#ifndef TWO_FTPD__H__
#define TWO_FTPD__H__

#include <sys/stat.h>
#include <sys/types.h>

#define CR ((char)015)
#define LF ((char)012)
#define SPACE ((char)040)
#define ESCAPE ((char)0377)

#ifndef BUFSIZE
#define BUFSIZE 4096
#endif

struct verb
{
  const char* name;
  int hideparam;
  int (*fn0)(void);
  int (*fn1)(void);
};
typedef struct verb verb;

extern const char* req_param;
extern unsigned req_param_len;
extern time_t now;
extern struct timeval timeout;

/* In backend.c */
const char* tcplocalip;
extern int handle_pass(void);

/* In listdir.c */
extern const char** listdir(const char* path);

/* In respond.c */
extern int respond(unsigned code, int final, const char* msg);

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

/* Used by main.c */
extern verb verbs[];
extern int startup(int argc, char* argv[]);

#endif
