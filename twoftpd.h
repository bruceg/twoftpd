#ifndef TWO_FTPD__H__
#define TWO_FTPD__H__

#include <sys/stat.h>
#include <sys/types.h>

#define DO_CHROOT 1

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
  int (*fn0)(void);
  int (*fn1)(void);
};
typedef struct verb verb;

extern const char* req_param;
extern unsigned req_param_len;
extern time_t now;
extern verb verbs[];

extern void format_stat(const struct stat*, const char* filename, char* out);
extern const char** listdir(const char* path);
extern int respond(unsigned code, int final, const char* msg);
extern int startup(int argc, char* argv[]);

#endif
