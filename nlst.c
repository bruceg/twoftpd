#include <unistd.h>
#include "twoftpd.h"

int handle_nlst(void)
{
  int fd;
  const char** entries;

  if ((entries = listdir(req_param ? req_param : ".")) == 0)
    return respond(550, 1, "Could not list directory.");

  if ((fd = make_connection()) == -1) return 1;

  while (*entries) {
    write(fd, *entries, strlen(*entries));
    write(fd, "\r\n", 2);
    ++entries;
  }
  close(fd);
  return respond(226, 1, "Transfer complete.");
}
