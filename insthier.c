#include "installer.h"

void insthier(void)
{
  c(bin, 0,      "twoftpd-anon",   -1, -1, 0755);
  c(bin, 0,      "twoftpd-auth",   -1, -1, 0755);
  c(bin, 0,      "twoftpd-xfer",   -1, -1, 0755);

  d(man,         "man1",           -1, -1, 0755);
  c(man, "man1", "twoftpd-auth.1", -1, -1, 0755);
  c(man, "man1", "twoftpd-xfer.1", -1, -1, 0755);
}
