#include <vtktiff/tiffDllConfig.h>

/* Compile with -DTIFFDLL for Windows DLL support */
#if (defined(_WIN32) || defined(__WIN32__)) && !defined(WIN32)
#  define WIN32
#endif
#if defined(__GNUC__) || defined(WIN32) || defined(__386__) || defined(i386)
#  ifndef __32BIT__
#    define __32BIT__
#  endif
#endif
#if defined(__MSDOS__) && !defined(MSDOS)
#  define MSDOS
#endif

#if defined(TIFFDLL)
#  if defined(_WINDOWS) || defined(WINDOWS)
#    ifdef FAR
#      undef FAR
#    endif
#    include <windows.h>
#    define TEXPORT  WINAPI
#    ifdef WIN32
#      define TEXPORTVA  WINAPIV
#    else
#      define TEXPORTVA  FAR _cdecl _export
#    endif
#  endif
#  if defined (__BORLANDC__)
#    if (__BORLANDC__ >= 0x0500) && defined (WIN32)
#      include <windows.h>
/*
#      define TEXPORT __declspec(dllexport) WINAPI
#      define TEXPORTRVA __declspec(dllexport) WINAPIV
*/
#      define TEXPORT _cdecl _export
#      define TEXPORTRVA _cdecl _export
#    else
#      if defined (_Windows) && defined (__DLL__)
#        define TEXPORT _export
#        define TEXPORTVA _export
#      endif
#    endif
#  endif
#endif
#ifndef TEXPORT
#  define TEXPORT
#endif
#ifndef TEXPORTVA
#  define TEXPORTVA
#endif
#ifndef TEXTERN
#  define TEXTERN extern
#endif

#ifndef FAR
#   define FAR
#endif

