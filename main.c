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
#include <iobuf/iobuf.h>
#include <unix/sig.h>
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
  else if (!ibuf_eof(&inbuf))
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
      case TELNET_DONT:
      case TELNET_WONT:
	saw_esc_ignore = *byte; break;
      case TELNET_WILL: saw_esc_respond = TELNET_DONT; break;
      case TELNET_DO  : saw_esc_respond = TELNET_WONT; break;
      case TELNET_IAC : request[offset++] = TELNET_IAC; break;
      }
    }
    else if (saw_esc_ignore) {
      saw_esc_ignore = 0;
    }
    else if (saw_esc_respond) {
      obuf_putc(&outbuf, TELNET_IAC);
      obuf_putc(&outbuf, saw_esc_respond);
      obuf_putc(&outbuf, *byte);
      obuf_flush(&outbuf);
      saw_esc_respond = 0;
    }
    else if (*byte == TELNET_IAC)
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

  /* Some firewalls appear to add extra CR bytes to some commands,
   * so strip all trailing CRs instead of just the last one. */
  while (request[length-1] == CR) --length;
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
  const command* cmd;

  cmd = find_command(table1);
  if (!cmd && table2) cmd = find_command(table2);
  if (!cmd) {
    if (log) log2(request, req_param ? req_param : "(no parameter)");
    return respond(502, 1, "Command not supported.");
  }
  
  if (req_param) {
    if (log) log2(cmd->name, cmd->hideparam ? "XXXXXXXX" : req_param);
    if (cmd->fn1)
      return cmd->fn1();
    return respond(501, 1, "Command requires no parameter");
  }
  else {
    if (log) log1(cmd->name);
    if (cmd->fn0)
      return cmd->fn0();
    return respond(504, 1, "Command requires a parameter");
  }
}

static void handle_alrm(int ignored)
{
  respond(421, 1, "Session timed out.");
  _exit(0);
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

  sig_alarm_catch(handle_alrm);
  if (!startup(argc, argv)) return 1;
  for (;;) {
    int len = read_request();
    if (len < 0) break;
    parse_request(len);
    if (!dispatch_request(internal_verbs, verbs, log_requests)) break;
  }
  return 0;
}
