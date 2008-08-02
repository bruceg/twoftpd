#ifndef TWO_FTPD__H__
#define TWO_FTPD__H__

#include <sys/stat.h>
#include <sys/types.h>

#define SPACE ((char)040)

#define TELNET_SE   ((char)240)	/* End subnegotiation parameters */
#define TELNET_NOP  ((char)241)	/* No operation */
#define TELNET_MARK ((char)242)	/* Data Mark */
#define TELNET_BRK  ((char)243)	/* NVT character BRK */
#define TELNET_IP   ((char)244)	/* Interrupt Process */
#define TELNET_AO   ((char)245)	/* Abort output */
#define TELNET_AYT  ((char)246)	/* Are You There */
#define TELNET_EC   ((char)247)	/* Erase character */
#define TELNET_EL   ((char)248)	/* Erase line */
#define TELNET_GA   ((char)249)	/* Go ahead */
#define TELNET_SB   ((char)250)	/* Start subnegotiation parameters */
#define TELNET_WILL ((char)251)
#define TELNET_WONT ((char)252)
#define TELNET_DO   ((char)253)
#define TELNET_DONT ((char)254)
#define TELNET_IAC  ((char)255)	/* Interpret as Command (escape) */

#ifndef BUFSIZE
#define BUFSIZE 4096
#endif

struct command
{
  const char* name;
  int hideparam;
  int (*fn0)(void);
  int (*fn1)(void);
};
typedef struct command command;

extern const char* req_param;
extern unsigned req_param_len;
extern unsigned timeout;

/* In banner.c */
extern void show_banner(unsigned code, const char* banner);

/* In respond.c */
extern int log_responses;
extern int respond_start(unsigned code, int final);
extern int respond_str(const char* msg);
extern int respond_uint(unsigned long num);
extern int respond_end(void);
extern int respond_syserr(unsigned code, const char *msg);
extern int respond(unsigned code, int final, const char* msg);
extern void respond_start_xfer(void);
extern int respond_xferresult(unsigned result, unsigned long bytes, int sent);

/* In responses.c */
extern int respond_internal_error(void);
extern int respond_ok(void);
extern int respond_permission_denied(void);

/* In strtou.c */
extern unsigned long strtou(const char* s, const char** end);

/* Used by main.c */
extern const command verbs[];
extern const command site_commands[];
extern int startup(int argc, char* argv[]);

#endif
