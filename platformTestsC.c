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
