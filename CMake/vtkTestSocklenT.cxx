#include <stdio.h>
#include <stdlib.h>
#if !defined(_WIN32) || defined(__CYGWIN__)
# include <sys/types.h>
# include <sys/socket.h>
#endif

int main()
{
  /* Test whether getsockname takes socklen_t.  */
  if(getsockname(0, 0, (socklen_t*)0)) return 0;
  if(sizeof (socklen_t)) return 0;
  return 0;
}
