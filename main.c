#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "iobuf/iobuf.h"
#include "twoftpd.h"

static char request[BUFSIZE];
static const char* req_verb;
const char* req_param;
unsigned req_param_len;
struct timeval timeout;

static pid_t pid = 0;
static const char* twoftpd = "twoftpd";

static void log1(const char* msg)
{
  if (!pid) pid = getpid();
  fprintf(stderr, "%s[%d]: %s\n", twoftpd, pid, msg);
}

static void log2(const char* msg1, const char* msg2)
{
  if (!pid) pid = getpid();
  fprintf(stderr, "%s[%d]: %s %s\n", twoftpd, pid, msg1, msg2);
}

static int handle_quit(void)
{
  respond(221, 1, "Bye.");
  return 0;
}

static int handle_help(void)
{
  return respond(502, 1, "No help is available.");
}

static int handle_syst(void)
{
  return respond(215, 1, "UNIX Type: L8");
}

static int handle_noop(void)
{
  return respond(200, 1, "Awaiting your commands, master...");
}

static verb internal_verbs[] = {
  { "QUIT", 0, handle_quit, 0 },
  { "HELP", 0, handle_help, 0 },
  { "SYST", 0, handle_syst, 0 },
  { "NOOP", 0, handle_noop, 0 },
  { 0,      0, 0,           0 }
};

static int read_request(void)
/* Returns number of bytes read before the LF, or -1 for EOF */
{
  unsigned offset;
  int saw_esc;
  int saw_esc_respond;
  int saw_esc_ignore;
  char byte[1];

  alarm(timeout.tv_sec);
  saw_esc = saw_esc_respond = saw_esc_ignore = 0;
  offset = 0;
  while (offset < sizeof request - 1) {
    if (!ibuf_getc(&inbuf, byte)) return -1;
    if (saw_esc) {
      saw_esc = 0;
      switch (*byte) {
      case (char)0376:
      case (char)0374: saw_esc_ignore = *byte; break;
      case (char)0373: saw_esc_respond = 0376; break;
      case (char)0375: saw_esc_respond = 0374; break;
      case (char)0377: request[offset++] = *byte; break;
      }
    }
    else if (saw_esc_ignore) {
      saw_esc_ignore = 0;
    }
    else if (saw_esc_respond) {
      obuf_putc(&outbuf, ESCAPE);
      obuf_putc(&outbuf, saw_esc_respond);
      obuf_putc(&outbuf, byte[0]);
      obuf_flush(&outbuf);
      saw_esc_respond = 0;
    }
    else if (*byte == ESCAPE)
      saw_esc = 1;
    else if (*byte == LF) {
      alarm(0);
      return offset;
    }
    else
      request[offset++] = *byte;
  }
  while (ibuf_getc(&inbuf, byte) && *byte != LF)
    ;
  alarm(0);
  return offset;
}

static void parse_request(unsigned length)
{
  char* ptr;
  char* end;

  if (request[length-1] == CR) --length;
  request[length] = 0;
  end = request + length;
  req_verb = request;
  req_param = 0;
  req_param_len = 0;
  for (ptr = request; ptr < end; ptr++) {
    if (*ptr == SPACE) break;
  }
  if (ptr == end) return;
  *ptr++ = 0;
  if (ptr == end) return;
  req_param = ptr;
  req_param_len = end - ptr;
}

static verb* find_verb(verb* verb)
{
  for (; verb->name; ++verb) {
    if (!strcasecmp(verb->name, req_verb))
      return verb;
  }
  return 0;
}

static int dispatch_request(void)
{
  verb* verb;

  verb = find_verb(internal_verbs);
  if (!verb) verb = find_verb(verbs);
  if (!verb) {
    log2(request, req_param ? req_param : "(no parameter)");
    return respond(502, 1, "Verb not supported.");
  }
  
  if (req_param) {
    log2(verb->name, verb->hideparam ? "XXXXXXXX" : req_param);
    if (verb->fn1)
      return verb->fn1();
    return respond(501, 1, "Verb requires no parameter");
  }
  else {
    log1(verb->name);
    if (verb->fn0)
      return verb->fn0();
    return respond(504, 1, "Verb requires a parameter");
  }
}

int main(int argc, char* argv[])
{
  char* tmp;
  
  tmp = getenv("TIMEOUT");
  if (!tmp || (timeout.tv_sec = atoi(tmp)) <= 0)
    timeout.tv_sec = 900;
  timeout.tv_usec = 0;
  
  if (!startup(argc, argv)) return 1;
  for (;;) {
    int len = read_request();
    if (len < 0) break;
    parse_request(len);
    if (!dispatch_request()) break;
  }
  return 0;
}
