#include <string.h>
#include <unistd.h>
#include "twoftpd.h"
#include "backend.h"

int binary_flag = 0;

int handle_type(void)
{
  if (!strcasecmp(req_param, "A") || !strcasecmp(req_param, "A N")) {
    binary_flag = 0;
    return respond(200, 1, "Transfer mode changed to ASCII.");
  }
  if (!strcasecmp(req_param, "I") || !strcasecmp(req_param, "L 8")) {
    binary_flag = 1;
    return respond(200, 1, "Transfer mode changed to BINARY.");
  }
  return respond(501, 1, "Unknown transfer type.");
}

int handle_stru(void)
{
  if (!strcasecmp(req_param, "F"))
    return respond(200, 1, "OK.");
  return respond(504, 1, "Invalid parameter.");
}

int handle_mode(void)
{
  if (!strcasecmp(req_param, "S"))
    return respond(200, 1, "OK.");
  return respond(504, 1, "Invalid parameter.");
}

int handle_cwd(void)
{
  if (chdir(req_param))
    return respond(550, 1, "Directory change failed.");
  return respond(250, 1, "Changed directory.");
}

int handle_pwd(void)
{
  size_t len;
  char buffer[BUFSIZE];
  if (!getcwd(buffer+1, sizeof buffer - 2))
    return respond(550, 1, "Could not determine current working directory.");
  buffer[0] = '"';
  len = strlen(buffer);
  buffer[len++] = '"';
  buffer[len] = 0;
  return respond(257, 1, buffer);
}

int handle_cdup(void)
{
  if (chdir(".."))
    return respond(550, 1, "Directory change failed.");
  return respond(257, 1, "Changed directory.");
}
