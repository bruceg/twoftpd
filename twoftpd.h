#ifndef TWO_FTPD__H__
#define TWO_FTPD__H__

#include <sys/stat.h>

#define CR 015
#define LF 012
#define SPACE 040

#ifndef BUFSIZE
#define BUFSIZE 4096
#endif

struct verb
{
  const char* name;
  int (*fn0)(void);
  int (*fn1)(void);
};
typedef struct verb verb;

extern const char* req_param;
extern unsigned req_param_len;

extern verb verbs[];

extern void format_stat(const struct stat*, const char* filename, char* out);
extern const char** listdir(const char* path);
extern int respond(unsigned code, int final, const char* msg);
extern int startup(int argc, char* argv[]);

#endif
