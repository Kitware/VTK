// .NAME vtkmyUnsortedWin32Header - manage Windows system differences
// .SECTION Description
// The vtkmyUnsortedWin32Header captures some system differences between Unix
// and Windows operating systems. 

#ifndef __vtkmyUnsortedWin32Header_h
#define __vtkmyUnsortedWin32Header_h

#include <vtkmyConfigure.h>

#if defined(WIN32) && !defined(VTK_MY_STATIC)
#if defined(vtkmyUnsorted_EXPORTS)
#define VTK_MY_UNSORTED_EXPORT __declspec( dllexport ) 
#else
#define VTK_MY_UNSORTED_EXPORT __declspec( dllimport ) 
#endif
#else
#define VTK_MY_UNSORTED_EXPORT
#endif

#endif
