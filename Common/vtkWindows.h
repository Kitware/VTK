/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWindows.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkWindows_h
#define __vtkWindows_h

/* This header is useless when not on windows or when the windows
   header has already been included at the top of VTK.  Block the
   whole thing on this condition.  */
#if defined(_WIN32) && !defined(VTK_INCLUDE_WINDOWS_H)

/*
Define some macros to shorten the windows header.  Record which ones
we defined here so that we can undefine them later.

See this page for details:
http://msdn.microsoft.com/library/en-us/vccore/html/_core_faster_builds_and_smaller_header_files.asp
*/
#if !defined(VTK_WINDOWS_FULL)
# if !defined(VC_EXTRALEAN)
#  define VC_EXTRALEAN
#  define VTK_WINDOWS_VC_EXTRALEAN
# endif
# if !defined(WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
#  define VTK_WINDOWS_WIN32_LEAN_AND_MEAN
# endif
# if !defined(NOSERVICE)
#  define NOSERVICE
#  define VTK_WINDOWS_NOSERVICE
# endif
# if !defined(NOMCX)
#  define NOMCX
#  define VTK_WINDOWS_NOMCX
# endif
# if !defined(NOIME)
#  define NOIME
#  define VTK_WINDOWS_NOIME
# endif
# if !defined(NONLS)
#  define NONLS
#  define VTK_WINDOWS_NONLS
# endif
#endif

/* Include the real windows header. */
#include <windows.h>

/* Undefine any macros we defined to shorten the windows header.
   Leave the VTK_WINDOWS_* versions defined so that user code can tell
   what parts of the windows header were included.  */
#if !defined(VTK_WINDOWS_FULL)
# if defined(VTK_WINDOWS_VC_EXTRALEAN)
#  undef VC_EXTRALEAN
# endif
# if defined(VTK_WINDOWS_WIN32_LEAN_AND_MEAN)
#  undef WIN32_LEAN_AND_MEAN
# endif
# if defined(VTK_WINDOWS_NOSERVICE)
#  undef NOSERVICE
# endif
# if defined(VTK_WINDOWS_NOMCX)
#  undef NOMCX
# endif
# if defined(VTK_WINDOWS_NOIME)
#  undef NOIME
# endif
# if defined(VTK_WINDOWS_NONLS)
#  undef NONLS
# endif
#endif

#endif /* defined(_WIN32) && !defined(VTK_INCLUDE_WINDOWS_H) */

#endif
