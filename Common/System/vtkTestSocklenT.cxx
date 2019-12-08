#include <cstdio>
#include <cstdlib>
#if !defined(_WIN32) || defined(__CYGWIN__)
#include <sys/socket.h>
#include <sys/types.h>
#endif

int main()
{
  /* Test whether getsockname takes socklen_t.  */
  struct sockaddr addr;
  socklen_t length = 0;
  if (getsockname(0, &addr, &length))
    return 0;
  if (sizeof(socklen_t))
    return 0;
  return 0;
}
