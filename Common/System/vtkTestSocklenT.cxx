#include <cstdio>
#include <cstdlib>
#if !defined(_WIN32) || defined(__CYGWIN__)
# include <sys/types.h>
# include <sys/socket.h>
#endif

int main()
{
  /* Test whether getsockname takes socklen_t.  */
  struct sockaddr addr;
  socklen_t length;
  if(getsockname(0, &addr, (socklen_t*)&length)) return 0;
  if(sizeof (socklen_t)) return 0;
  return 0;
}
