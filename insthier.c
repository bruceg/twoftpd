#include "conf_bin.c"
#include "conf_man.c"
#include <installer.h>

void insthier(void)
{
  int bin = opendir(conf_bin);
  int man = opendir(conf_man);
  int man1 = d(man, "man1", -1, -1, 0755);
  
  c(bin,  "twoftpd-anon",      -1, -1, 0555);
  c(bin,  "twoftpd-anon-conf", -1, -1, 0555);
  c(bin,  "twoftpd-auth",      -1, -1, 0555);
  c(bin,  "twoftpd-bind-port", -1, -1, 0555);
  c(bin,  "twoftpd-conf",      -1, -1, 0555);
  c(bin,  "twoftpd-drop",      -1, -1, 0555);
  c(bin,  "twoftpd-switch",    -1, -1, 0555);
  c(bin,  "twoftpd-xfer",      -1, -1, 0555);

  c(man1, "twoftpd-auth.1",    -1, -1, 0444);
  c(man1, "twoftpd-switch.1",  -1, -1, 0444);
  c(man1, "twoftpd-xfer.1",    -1, -1, 0444);
}
