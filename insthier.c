#include "conf_bin.c"
#include "conf_man.c"
#include "installer.h"

void insthier(void)
{
  int bin;
  int man;
  
  bin = opendir(conf_bin);
  man = opendir(conf_man);
  
  c(bin, 0,      "twoftpd-anon",      -1, -1, 0755);
  c(bin, 0,      "twoftpd-anon-conf", -1, -1, 0755);
  c(bin, 0,      "twoftpd-auth",      -1, -1, 0755);
  c(bin, 0,      "twoftpd-conf",      -1, -1, 0755);
  c(bin, 0,      "twoftpd-xfer",      -1, -1, 0755);

  d(man,         "man1",              -1, -1, 0755);
  c(man, "man1", "twoftpd-auth.1",    -1, -1, 0644);
  c(man, "man1", "twoftpd-xfer.1",    -1, -1, 0644);
}
