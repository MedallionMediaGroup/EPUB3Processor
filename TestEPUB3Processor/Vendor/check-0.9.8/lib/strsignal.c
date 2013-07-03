#include "libcompat.h"

const char *
strsignal (int sig)
{
  static char signame[40];
  sprintf(signame, "SIG #%d", sig);
  return signame;
}
