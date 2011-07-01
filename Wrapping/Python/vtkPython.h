/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPython.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkPython_h
#define __vtkPython_h

/* Undefine macros that Python.h defines to avoid redefinition warning.  */
#undef _POSIX_C_SOURCE
#undef _POSIX_THREADS
#undef _XOPEN_SOURCE // Only defined by Python.h in python 2.3 and up

#include "vtkConfigure.h"
#include "vtkABI.h"

#if defined(WIN32)
# if defined(vtkPythonCore_EXPORTS)
#  define VTK_PYTHON_EXPORT VTK_ABI_EXPORT
# else
#  define VTK_PYTHON_EXPORT VTK_ABI_IMPORT
# endif
# if defined(vtkCommonPythonD_EXPORTS)
#  define VTK_COMMON_PYTHON_EXPORT VTK_ABI_EXPORT
# else
#  define VTK_COMMON_PYTHON_EXPORT VTK_ABI_IMPORT
# endif
#else
# define VTK_PYTHON_EXPORT
# define VTK_COMMON_PYTHON_EXPORT
#endif

/*
   Use the real python debugging library if it is provided.
   Otherwise use the "documented" trick involving checking for _DEBUG
   and undefined that symbol while we include Python headers.
   Update: this method does not fool Microsoft Visual C++ 8 anymore; two
   of its header files (crtdefs.h and use_ansi.h) check if _DEBUG was set
   or not, and set flags accordingly (_CRT_MANIFEST_RETAIL,
   _CRT_MANIFEST_DEBUG, _CRT_MANIFEST_INCONSISTENT). The next time the
   check is performed in the same compilation unit, and the flags are found,
   and error is triggered. Let's prevent that by setting _CRT_NOFORCE_MANIFEST.
*/
#if defined(VTK_WINDOWS_PYTHON_DEBUGGABLE)
# include <Python.h>
#else
# ifdef _DEBUG
// Include these low level headers before undefing _DEBUG. Otherwise when doing
// a debug build against a release build of python the compiler will end up 
// including these low level headers without DEBUG enabled, causing it to try 
// and link release versions of this low level C api.
# include <basetsd.h>
# include <assert.h>
# include <ctype.h>
# include <errno.h>
# include <io.h>
# include <math.h>
# include <sal.h>
# include <stdarg.h>
# include <stddef.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/stat.h>
# include <time.h>
# include <wchar.h>
#  undef _DEBUG
#  if defined(_MSC_VER) && _MSC_VER >= 1400
#    define _CRT_NOFORCE_MANIFEST 1
#  endif
#  include <Python.h>
#  define _DEBUG
# else
#  include <Python.h>
# endif
#endif

#endif
