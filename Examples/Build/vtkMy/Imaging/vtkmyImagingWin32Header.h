// .NAME vtkmyImagingWin32Header - manage Windows system differences
// .SECTION Description
// The vtkmyImagingWin32Header captures some system differences between Unix
// and Windows operating systems. 

#ifndef __vtkmyImagingWin32Header_h
#define __vtkmyImagingWin32Header_h

#include <vtkmyConfigure.h>

#if defined(WIN32) && !defined(VTK_MY_STATIC)
#if defined(vtkmyImaging_EXPORTS)
#define VTK_MY_IMAGING_EXPORT __declspec( dllexport ) 
#else
#define VTK_MY_IMAGING_EXPORT __declspec( dllimport ) 
#endif
#else
#define VTK_MY_IMAGING_EXPORT
#endif

#endif
