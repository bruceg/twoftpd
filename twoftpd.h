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
extern unsigned timeout;
extern int log_requests;

/* In banner.c */
extern void show_banner(unsigned code, const char* banner);

/* In respond.c */
extern int log_responses;
extern int respond_start(unsigned code, int final);
extern int respond_str(const char* msg);
extern int respond_end(void);
extern int respond(unsigned code, int final, const char* msg);

/* In strtou.c */
extern unsigned long strtou(const char* str, const char** end);

/* Used by main.c */
extern verb verbs[];
extern int startup(int argc, char* argv[]);

#endif
