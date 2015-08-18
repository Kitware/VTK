#define SIMPLE_TEST(x) int main(){ x; return 0; }

#ifdef HAVE_C99_DESIGNATED_INITIALIZER

#ifdef FC_DUMMY_MAIN
#ifndef FC_DUMMY_MAIN_EQ_F77
#  ifdef __cplusplus
extern "C"
#  endif
int FC_DUMMY_MAIN()
{ return 1;}
#endif
#endif
int
main ()
{

  typedef struct
  {
    int x;
    union
    {
      int i;
      double d;
    }u;
  }di_struct_t;
  di_struct_t x =
  { 0,
    { .d = 0.0}};
  ;
  return 0;
}

#endif

#ifdef HAVE_C99_FUNC

#ifdef FC_DUMMY_MAIN
#ifndef FC_DUMMY_MAIN_EQ_F77
#  ifdef __cplusplus
     extern "C"
#  endif
   int FC_DUMMY_MAIN() { return 1; }
#endif
#endif
int
main ()
{
 const char *fname = __func__;
  ;
  return 0;
}

#endif

#ifdef VSNPRINTF_WORKS
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

int test_vsnprintf(const char *fmt,...)
{
    va_list     ap;
    char *s = malloc(16);
    int ret;

    va_start(ap, fmt);
    ret=vsnprintf(s,16,"%s",ap);
    va_end(ap);

    return(ret!=42 ? 1 : 0);
}

int main(void)
{
    return(test_vsnprintf("%s","A string that is longer than 16 characters"));
}
#endif


#ifdef TIME_WITH_SYS_TIME
/* Time with sys/time test */

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

int
main ()
{
if ((struct tm *) 0)
return 0;
  ;
  return 0;
}

#endif

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
int main() { return 0; }
#endif /* STDC_HEADERS */

#ifdef HAVE_TM_ZONE

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(struct tm tm; tm.tm_zone);

#endif /* HAVE_TM_ZONE */

#ifdef HAVE_STRUCT_TM_TM_ZONE

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(struct tm tm; tm.tm_zone);

#endif /* HAVE_STRUCT_TM_TM_ZONE */

#ifdef HAVE_ATTRIBUTE

#if 0
static void test int __attribute((unused)) var)
{
  int __attribute__((unused)) x = var;
}

int main(void)
{
  test(19);
}

#else
int
main ()
{
int __attribute__((unused)) x
  ;
  return 0;
}
#endif


#endif /* HAVE_ATTRIBUTE */

#ifdef HAVE_FUNCTION

#ifdef FC_DUMMY_MAIN
#ifndef FC_DUMMY_MAIN_EQ_F77
#  ifdef __cplusplus
     extern "C"
#  endif
   int FC_DUMMY_MAIN() { return 1; }
#endif
#endif
int
main ()
{
(void)__FUNCTION__
  ;
  return 0;
}

#endif /* HAVE_FUNCTION */

#ifdef HAVE_TM_GMTOFF

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(struct tm tm; tm.tm_gmtoff=0);

#endif /* HAVE_TM_GMTOFF */

#ifdef HAVE___TM_GMTOFF

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(struct tm tm; tm.__tm_gmtoff=0);

#endif /* HAVE_TM_GMTOFF */

#ifdef HAVE_TIMEZONE

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(timezone=0);

#endif /* HAVE_TIMEZONE */

#ifdef HAVE_STRUCT_TIMEZONE

#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
SIMPLE_TEST(struct timezone tz; tz.tz_minuteswest=0);

#endif /* HAVE_STRUCT_TIMEZONE */

#ifdef HAVE_STAT_ST_BLOCKS

#include <sys/stat.h>
SIMPLE_TEST(struct stat sb; sb.st_blocks=0);

#endif /* HAVE_STAT_ST_BLOCKS */

#ifdef PRINTF_LL_WIDTH

#ifdef HAVE_LONG_LONG
#  define LL_TYPE long long
#else /* HAVE_LONG_LONG */
#  define LL_TYPE __int64
#endif /* HAVE_LONG_LONG */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER) && defined(_DEBUG)
# include <crtdbg.h>
int DebugReport(int reportType, char* message, int* returnValue)
{
  (void)reportType;
  (void)message;
  (void)returnValue;
  return 1; /* no further handling required */
}
#endif

int main(void)
{
  char *llwidthArgs[] = { "I64", "l64", "l", "L", "q", "ll", NULL };
  char *s = malloc(128);
  char **currentArg = NULL;
  LL_TYPE x = (LL_TYPE)1048576 * (LL_TYPE)1048576;
#if defined(_MSC_VER) && defined(_DEBUG)
  _CrtSetReportHook(DebugReport);
#endif
  for (currentArg = llwidthArgs; *currentArg != NULL; currentArg++)
    {
    char formatString[64];
    sprintf(formatString, "%%%sd", *currentArg);
    sprintf(s, formatString, x);
    if (strcmp(s, "1099511627776") == 0)
      {
      printf("PRINTF_LL_WIDTH=[%s]\n", *currentArg);
      return 0;
      }
    }
  return 1;
}

#endif /* PRINTF_LL_WIDTH */

#ifdef SYSTEM_SCOPE_THREADS
#include <stdlib.h>
#include <pthread.h>

int main(void)
{
    pthread_attr_t attribute;
    int ret;

    pthread_attr_init(&attribute);
    ret=pthread_attr_setscope(&attribute, PTHREAD_SCOPE_SYSTEM);
    if (ret==0)
        return 0;
    return 1;
}

#endif /* SYSTEM_SCOPE_THREADS */

#ifdef HAVE_SOCKLEN_T

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

SIMPLE_TEST(socklen_t foo);

#endif /* HAVE_SOCKLEN_T */

#ifdef DEV_T_IS_SCALAR

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

int main ()
{
  dev_t d1, d2;
  if(d1==d2)
    return 0;
  return 1;
}

#endif /* DEV_T_IS_SCALAR */

#ifdef HAVE_OFF64_T
#include <sys/types.h>
int main()
{
  off64_t n = 0;
  return (int)n;
}
#endif

#ifdef HAVE_STAT64_STRUCT
#include <sys/types.h>
#include <sys/stat.h>],
struct stat64 sb;
int main()
{
  return 0;
}
#endif

#ifdef TEST_DIRECT_VFD_WORKS
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(void)
{
   int fid;
   if((fid=open("tst_file", O_CREAT | O_TRUNC | O_DIRECT, 0755))<0)
       return 1;
   close(fid);
   remove("tst_file");
   return 0;
}
#endif

#ifdef HAVE_DIRECT
       SIMPLE_TEST(posix_memalign());
#endif

#ifdef TEST_LFS_WORKS
/* Return 0 when LFS is available and 1 otherwise.  */
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _LARGE_FILES
#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char **argv)
{
  /* check that off_t can hold 2^63 - 1 and perform basic operations... */
#define OFF_T_64 (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
  if (OFF_T_64 % 2147483647 != 1)
    return 1;

  /* stat breaks on SCO OpenServer */
  struct stat buf;
  stat( argv[0], &buf );
  if (!S_ISREG(buf.st_mode))
    return 2;

  FILE *file = fopen( argv[0], "r" );
  off_t offset = ftello( file );
  fseek( file, offset, SEEK_CUR );
  fclose( file );
  return 0;
}
#endif

#ifdef GETTIMEOFDAY_GIVES_TZ
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
int main(void)
{
 struct timeval tv;
 struct timezone tz;
 tz.tz_minuteswest = 7777;  /* Initialize to an unreasonable number */
 tz.tz_dsttime = 7;
 gettimeofday(&tv, &tz);
    /* Check whether the function returned any value at all */
 if(tz.tz_minuteswest == 7777 && tz.tz_dsttime == 7)
     return 1;
 else return 0;
}
#endif

#ifdef LONE_COLON
int main(int argc, char * argv) 
{
  return 0;
}
#endif

#ifdef CXX_HAVE_OFFSETOF

#include <stdio.h>
#include <stddef.h>

#ifdef FC_DUMMY_MAIN
#ifndef FC_DUMMY_MAIN_EQ_F77
#  ifdef __cplusplus
extern "C"
#  endif
int FC_DUMMY_MAIN()
{ return 1;}
#endif
#endif
int
main ()
{

  struct index_st
  {
    unsigned char type;
    unsigned char num;
    unsigned int len;
  };
  typedef struct index_st index_t;
  int x,y;
  x = offsetof(struct index_st, len);
  y = offsetof(index_t, num)

  ;
  return 0;
}

#endif

#ifdef HAVE_IOEO

#include <windows.h>
typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
int main ()
{
	PGNSI pGNSI;
	pGNSI = (PGNSI) GetProcAddress(
      GetModuleHandle(TEXT("kernel32.dll")), 
      "InitOnceExecuteOnce");
	if(NULL == pGNSI)
		return 1;
	else
		return 0;
}

#endif /* HAVE_IOEO */

#ifdef HAVE_STRUCT_VIDEOCONFIG

SIMPLE_TEST(struct videoconfig w; w.numtextcols=0);

#endif /* HAVE_TM_GMTOFF */

#ifdef HAVE_STRUCT_TEXT_INFO

SIMPLE_TEST(struct text_info w; w.screenwidth=0);

#endif /* HAVE_TM_GMTOFF */


#if defined( INLINE_TEST_inline ) || defined( INLINE_TEST___inline__ ) || defined( INLINE_TEST___inline )
#ifndef __cplusplus
typedef int foo_t;
static INLINE_TEST_INLINE foo_t static_foo () { return 0; }
INLINE_TEST_INLINE foo_t foo () {return 0; }
int main() { return 0; }
#endif

#endif /* INLINE_TEST */
