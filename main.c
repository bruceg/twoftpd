#include <string.h>
#include <unistd.h>
#include "twoftpd.h"

static char request[4096];
static const char* req_verb;
const char* req_param;
unsigned req_param_len;
time_t now;

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
  { "QUIT", handle_quit, 0 },
  { "HELP", handle_help, 0 },
  { "SYST", handle_syst, 0 },
  { "NOOP", handle_noop, 0 },
  { 0, 0, 0 }
};

static int read_request(void)
/* Returns number of bytes read before the LF, or -1 for EOF */
{
  unsigned offset;
  int state;
  char byte[1];
  
  /* States:
   * 0 - reading data
   * 1 - read CR
   */
  state = 0;
  offset = 0;
  while (offset < sizeof request - 1) {
    if (read(0, byte, 1) != 1) return -1;
    switch (*byte) {
    case LF:
      request[offset] = 0;
      return offset;
    case CR:
      if (state == 1)
	request[offset++] = CR;
      else
	state = 1;
      break;
    default:
      if (state == 1) request[offset++] = CR;
      request[offset++] = *byte;
    }
  }
  while (read(0, byte, 1) == 1 && *byte != LF)
    ;
  request[offset] = 0;
  return offset;
}

static void parse_request(unsigned length)
{
  char* ptr;
  char* end;
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
  if (!verb) return respond(502, 1, "Verb not supported.");
  
  if (req_param) {
    if (verb->fn1)
      return verb->fn1();
    return respond(501, 1, "Verb requires no parameter");
  }
  else {
    if (verb->fn0)
      return verb->fn0();
    return respond(504, 1, "Verb requires a parameter");
  }
}

int main(int argc, char* argv[])
{
  if (!startup(argc, argv)) return 1;
  for (;;) {
    int len = read_request();
    if (len < 0) break;
    parse_request(len);
    if (!dispatch_request()) break;
  }
  return 0;
}
