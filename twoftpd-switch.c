#include <sysdeps.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "twoftpd.h"
#include "conf_bin.c"

const char program[] = "twoftpd-switch";
const int msg_show_pid = 1;

int main(void)
{
  char cmd[sizeof conf_bin + 13];
  char *argv[2] = { cmd, 0 };
  const char *shell;
  strcpy(cmd, conf_bin);
  if ((shell = getenv("SHELL")) != 0) {
    if (access(shell, X_OK) == 0)
      strcpy(cmd + sizeof conf_bin - 1, "/twoftpd-xfer");
    else if (errno != ENOENT && errno != EACCES) {
      respond_syserr(421, "Could not stat $SHELL");
      return 111;
    }
  }
  if (cmd[sizeof conf_bin - 1] == 0)
    strcpy(cmd + sizeof conf_bin - 1, "/twoftpd-anon");
  execv(argv[0], argv);
  respond_syserr(421, "Could not exec backend");
  return 111;
}
