// .NAME vtkmyCommonWin32Header - manage Windows system differences
// .SECTION Description
// The vtkmyCommonWin32Header captures some system differences between Unix
// and Windows operating systems. 

#ifndef __vtkmyCommonWin32Header_h
#define __vtkmyCommonWin32Header_h

#include <vtkmyConfigure.h>

#if defined(WIN32) && !defined(VTK_MY_STATIC)
#if defined(vtkmyCommon_EXPORTS)
#define VTK_MY_COMMON_EXPORT __declspec( dllexport ) 
#else
#define VTK_MY_COMMON_EXPORT __declspec( dllimport ) 
#endif
#else
#define VTK_MY_COMMON_EXPORT
#endif

#endif
