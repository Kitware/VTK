/*
  Macros to define main() in a cross-platform way.

  Usage:

    int PLATFORM_TEST_C_MAIN()
    {
      return 0;
    }

    int PLATFORM_TEST_C_MAIN_ARGS(argc, argv)
    {
      (void)argc; (void)argv;
      return 0;
    }
*/
#if defined(__CLASSIC_C__)
# define PLATFORM_TEST_C_MAIN() \
  main()
# define PLATFORM_TEST_C_MAIN_ARGS(argc, argv) \
  main(argc,argv) int argc; char* argv[];
#else
# define PLATFORM_TEST_C_MAIN() \
  main(void)
# define PLATFORM_TEST_C_MAIN_ARGS(argc, argv) \
  main(int argc, char* argv[])
#endif

/*--------------------------------------------------------------------------*/
#ifdef TEST_SUPPORT_IP6
#include <sys/socket.h>
#include <sys/types.h>

int PLATFORM_TEST_C_MAIN()
{
  struct sockaddr_storage ss;
  socket(AF_INET6, SOCK_STREAM, 0);
  return 0;
}
#endif

/*--------------------------------------------------------------------------*/
#ifdef TEST_HAVE_GETADDRINFO
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

int PLATFORM_TEST_C_MAIN()
{
  struct addrinfo hints, *ai;
  int error;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
#ifndef getaddrinfo
  (void)getaddrinfo;
#endif
  error = getaddrinfo("127.0.0.1", "8080", &hints, &ai);
  if(error)
    {
    return 1;
    }
  return 0;
}
#endif

/*--------------------------------------------------------------------------*/
#ifdef TEST_HAVE_SYS_DIR_H
#include <sys/types.h>
#include <sys/dir.h>
int PLATFORM_TEST_C_MAIN()
{
  if((DIR*)0) return 0;
  return 0;
}
#endif

/*--------------------------------------------------------------------------*/
#ifdef TEST_HAVE_SYS_NDIR_H
#include <sys/types.h>
#include <sys/ndir.h>
int PLATFORM_TEST_C_MAIN()
{
  if((DIR*)0) return 0;
  return 0;
}
#endif

/*--------------------------------------------------------------------------*/
#ifdef TEST_HAVE_VA_COPY
#include <stdarg.h>
va_list ap1,ap2;
int PLATFORM_TEST_C_MAIN()
{
  va_copy(ap1,ap2);
  return 0;
}
#endif

/*--------------------------------------------------------------------------*/
#ifdef TEST_HAVE___VA_COPY
#include <stdarg.h>
va_list ap1,ap2;
int PLATFORM_TEST_C_MAIN()
{
  __va_copy(ap1,ap2);
  return 0;
}
#endif

/*--------------------------------------------------------------------------*/
#ifdef TEST_HAVE_SOCKLEN_T
#include <sys/types.h>
#include <sys/socket.h>
int PLATFORM_TEST_C_MAIN()
{
  if((socklen_t*)0) return 0;
  if(sizeof(socklen_t)) return 0;
  return 0;
}
#endif
