/*
   This program accepts one argument which must be one of the following:
   
   PATH_SEP    return the path separator for the platform
   PATH_SLASH  return the directory separator
   LDD         return the name of the "ldd" equivalent for the platform
   LDD_FLAGS   return the flags (if any) needed for ldd equivalent
   LDPATH      return the name of the environment variable for shared lib path
*/


/* The path separator for this platform.  */
#if defined(_WIN32) && !defined(__CYGWIN__)
# define CMAKE_SHARED_PATH_SEP ";"
# define CMAKE_SHARED_PATH_SLASH "\\"
#else
# define CMAKE_SHARED_PATH_SEP ":"
# define CMAKE_SHARED_PATH_SLASH "/"
#endif

/* Select the environment variable holding the shared library runtime
   search path for this platform and build configuration.  Also select
   ldd command equivalent.  */

/* Linux */
#if defined(__linux)
# define CMAKE_SHARED_LDD "ldd"
# define CMAKE_SHARED_LDD_FLAGS ""
# define CMAKE_SHARED_LDPATH "LD_LIBRARY_PATH"
# define RETURN_VALUE 0
#endif

/* FreeBSD */
#if defined(__FreeBSD__)
# define CMAKE_SHARED_LDD "ldd"
# define CMAKE_SHARED_LDD_FLAGS ""
# define CMAKE_SHARED_LDPATH "LD_LIBRARY_PATH"
# define RETURN_VALUE 0
#endif

/* OSX */
#if defined(__APPLE__)
# define CMAKE_SHARED_LDD "otool"
# define CMAKE_SHARED_LDD_FLAGS "-L"
# define CMAKE_SHARED_LDPATH "DYLD_LIBRARY_PATH"
# define RETURN_VALUE 0
#endif

/* AIX */
#if defined(_AIX)
# define CMAKE_SHARED_LDD "dump"
# define CMAKE_SHARED_LDD_FLAGS "-H"
# define CMAKE_SHARED_LDPATH "LIBPATH"
# define RETURN_VALUE 0
#endif

/* SUN */
#if defined(__sparc)
# define CMAKE_SHARED_LDD "ldd"
# define CMAKE_SHARED_LDD_FLAGS ""
# include <sys/isa_defs.h>
# if defined(_ILP32)
#  define CMAKE_SHARED_LDPATH "LD_LIBRARY_PATH"
#  define RETURN_VALUE 0
# elif defined(_LP64)
#  define CMAKE_SHARED_LDPATH "LD_LIBRARY_PATH_64"
#  define RETURN_VALUE 64
# endif
#endif

/* HP-UX */
#if defined(__hpux)
# define CMAKE_SHARED_LDD "chatr"
# define CMAKE_SHARED_LDD_FLAGS ""
# if defined(__LP64__)
#  define CMAKE_SHARED_LDPATH "LD_LIBRARY_PATH"
#  define RETURN_VALUE 64
# else
#  define CMAKE_SHARED_LDPATH "SHLIB_PATH"
#  define RETURN_VALUE 0
# endif
#endif

/* SGI MIPS */
#if defined(__sgi) && defined(_MIPS_SIM)
# define CMAKE_SHARED_LDD "ldd"
# define CMAKE_SHARED_LDD_FLAGS ""
# if _MIPS_SIM == _ABIO32
#  define CMAKE_SHARED_LDPATH "LD_LIBRARY_PATH"
#  define RETURN_VALUE 0
# elif _MIPS_SIM == _ABIN32
#  define CMAKE_SHARED_LDPATH "LD_LIBRARYN32_PATH"
#  define RETURN_VALUE 32
# elif _MIPS_SIM == _ABI64
#  define CMAKE_SHARED_LDPATH "LD_LIBRARY64_PATH"
#  define RETURN_VALUE 64
# endif
#endif

/* Windows */
#if defined(_WIN32)
# if defined(__CYGWIN__)
#  define CMAKE_SHARED_LDD "cygcheck"
#  define CMAKE_SHARED_LDD_FLAGS ""
# endif
# define CMAKE_SHARED_LDPATH "PATH"
# define RETURN_VALUE 0
#endif

/* Guess on this unknown system.  */
#if !defined(CMAKE_SHARED_LDPATH)
# define CMAKE_SHARED_LDD "ldd"
# define CMAKE_SHARED_LDD_FLAGS ""
# define CMAKE_SHARED_LDPATH "LD_LIBRARY_PATH"
#endif

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
  if (argc > 1)
    {
    if (strcmp("LDPATH", argv[1]) == 0)
      {
      fprintf(stdout, "%s\n", CMAKE_SHARED_LDPATH);
      return 0;
      }
    else if (strcmp("PATH_SEP", argv[1]) == 0)
      {
      fprintf(stdout, "%s\n", CMAKE_SHARED_PATH_SEP);
      return 0;
      }
    else if (strcmp("PATH_SLASH", argv[1]) == 0)
      {
      fprintf(stdout, "%s\n", CMAKE_SHARED_PATH_SLASH);
      return 0;
      }
    else if (strcmp("LDD", argv[1]) == 0)
      {
      fprintf(stdout, "%s\n", CMAKE_SHARED_LDD);
      return 0;
      }
    else if (strcmp("LDD_FLAGS", argv[1]) == 0)
      {
      fprintf(stdout, "%s\n", CMAKE_SHARED_LDD_FLAGS);
      return 0;
      }
   }

  fprintf(stdout, "\nusage: %s <item>   where item is one of the following:\n\n",
          argv[0]);
  fprintf(stdout, "  LDPATH      \"%s\"\n", CMAKE_SHARED_LDPATH);
  fprintf(stdout, "  PATH_SEP    \"%s\"\n", CMAKE_SHARED_PATH_SEP);
  fprintf(stdout, "  PATH_SLASH  \"%s\"\n", CMAKE_SHARED_PATH_SLASH); 
  fprintf(stdout, "  LDD         \"%s\"\n", CMAKE_SHARED_LDD);
  fprintf(stdout, "  LDD_FLAGS   \"%s\"\n", CMAKE_SHARED_LDD_FLAGS);
  fprintf(stdout, "\n");

  return 1;
}

                       
