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

/* In respond.c */
extern int respond_start(unsigned code, int final);
extern int respond(unsigned code, int final, const char* msg);
extern int respond_end(void);

/* Used by main.c */
extern verb verbs[];
extern int startup(int argc, char* argv[]);

#endif
