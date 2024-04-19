// XGetopt.h  Version 1.2
//
// Author:  Hans Dietrich
//          hdietrich2@hotmail.com
//
// This software is released into the public domain.
// You are free to use it in any way you like.
//
// This software is provided "as is" with no expressed
// or implied warranty.  I accept no liability for any
// damage or loss of business that this software may cause.
//
///////////////////////////////////////////////////////////////////////////////


#ifndef XGETOPT_H
#define XGETOPT_H

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#if 0
#ifdef _MSC_VER
# ifdef DLL_EXPORT
#  define GTOPT_EXTRA __declspec(dllexport)
# else
#  define GTOPT_EXTRA __declspec(dllimport)
# endif
#else
#  define GTOP_EXTRA
#endif
#endif

#define GTOPT_EXTRA
GTOPT_EXTRA extern int optind, opterr;
GTOPT_EXTRA extern TCHAR* optarg;
GTOPT_EXTRA extern int getopt(int argc, TCHAR *argv[], TCHAR *optstring);

#endif //XGETOPT_H
