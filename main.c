/* main.c - twoftpd client command parsing main loop
 * Copyright (C) 2001  Bruce Guenter <bruceg@em.ca>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "iobuf/iobuf.h"
#include "twoftpd.h"
#include "log.h"

static char request[BUFSIZE];
static const char* req_verb;
const char* req_param;
unsigned req_param_len;
unsigned timeout;

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

static int dispatch_request(const command* table1, const command* table2, int);
static int handle_site(void)
{
  char* ptr;
  if ((ptr = memchr(req_param, SPACE, req_param_len)) == 0) {
    req_verb = req_param;
    req_param = 0;
  }
  else {
    *ptr++ = 0;
    while (*ptr == SPACE) ++ptr;
    req_verb = req_param;
    req_param_len -= ptr - req_param;
    req_param = ptr;
  }
  return dispatch_request(site_commands, 0, 0);
}

static command internal_verbs[] = {
  { "SITE", 0, 0,           handle_site },
  { "QUIT", 0, handle_quit, 0 },
  { "HELP", 0, handle_help, 0 },
  { "SYST", 0, handle_syst, 0 },
  { "NOOP", 0, handle_noop, 0 },
  { 0,      0, 0,           0 }
};

static void inbuf_errmsg(void)
{
  if (ibuf_timedout(&inbuf))
    respond(421, 1, "Timed out waiting for command.");
  else
    respond_syserr(421, "I/O error");
}

static int read_request(void)
/* Returns number of bytes read before the LF, or -1 for EOF */
{
  unsigned offset;
  int saw_esc;
  int saw_esc_respond;
  int saw_esc_ignore;
  char byte[1];

  saw_esc = saw_esc_respond = saw_esc_ignore = 0;
  offset = 0;
  while (offset < sizeof request - 1) {
    if (!ibuf_getc(&inbuf, byte)) { inbuf_errmsg(); return -1; }
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
    else if (*byte == LF)
      break;
    else
      request[offset++] = *byte ? *byte : LF;
  }
  while (*byte != LF)
    if (!ibuf_getc(&inbuf, byte)) { inbuf_errmsg(); return -1; }
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

static const command* find_command(const command* table)
{
  for (; table->name; ++table) {
    if (!strcasecmp(table->name, req_verb))
      return table;
  }
  return 0;
}

static int dispatch_request(const command* table1, const command* table2,
			    int log)
{
  const command* command;

  command = find_command(table1);
  if (!command && table2) command = find_command(table2);
  if (!command) {
    if (log) log2(request, req_param ? req_param : "(no parameter)");
    return respond(502, 1, "Command not supported.");
  }
  
  if (req_param) {
    if (log) log2(command->name, command->hideparam ? "XXXXXXXX" : req_param);
    if (command->fn1)
      return command->fn1();
    return respond(501, 1, "Command requires no parameter");
  }
  else {
    if (log) log1(command->name);
    if (command->fn0)
      return command->fn0();
    return respond(504, 1, "Command requires a parameter");
  }
}

int main(int argc, char* argv[])
{
  const char* tmp;
  const char* end;
  int log_requests;
  
  log_requests = getenv("LOGREQUESTS") != 0;
  log_responses = getenv("LOGRESPONSES") != 0;

  tmp = getenv("TIMEOUT");
  if (tmp) {
    if ((timeout = strtou(tmp, &end)) == 0 || *end != 0) {
      respond(421, 1, "Configuration error, invalid timeout value");
      return 1;
    }
  }
  else
    timeout = 900;
  inbuf.io.timeout = timeout * 1000;
  outbuf.io.timeout = timeout * 1000;

  if (!startup(argc, argv)) return 1;
  for (;;) {
    int len = read_request();
    if (len < 0) break;
    parse_request(len);
    if (!dispatch_request(internal_verbs, verbs, log_requests)) break;
  }
  return 0;
}
