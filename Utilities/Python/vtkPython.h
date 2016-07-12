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
#ifndef vtkPython_h
#define vtkPython_h

#include "vtkPythonConfigure.h"
#include "vtkConfigure.h"
#include "vtkABI.h"

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
#if defined(_DEBUG) && !defined(VTK_WINDOWS_PYTHON_DEBUGGABLE)
# define VTK_PYTHON_UNDEF_DEBUG
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
# include <stdarg.h>
# include <stddef.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/stat.h>
# include <time.h>
# include <wchar.h>
# undef _DEBUG
# if defined(_MSC_VER)
#  define _CRT_NOFORCE_MANIFEST 1
# endif
#endif

/* We used to try to #undef feature macros that Python.h defines
to avoid re-definition warnings.  However, such warnings usually
indicate a violation of Python's documented inclusion policy:

 "Since Python may define some pre-processor definitions which
  affect the standard headers on some systems, you must include
  Python.h before any standard headers are included."
 (http://docs.python.org/c-api/intro.html#include-files)

To avoid re-definitions warnings, ensure "vtkPython.h" is included
before _any_ headers that define feature macros, whether or not
they are system headers.  Do NOT add any #undef lines here.  */

#if defined(_MSC_VER)
# pragma warning (push, 1)
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1800
#define HAVE_ROUND 1
#endif

#include <Python.h>

#if defined(_MSC_VER) && _MSC_VER >= 1800
#undef HAVE_ROUND
#endif

#if defined(_MSC_VER)
# pragma warning (pop)
#endif

#ifdef VTK_PYTHON_UNDEF_DEBUG
# define _DEBUG 1
# undef VTK_PYTHON_UNDEF_DEBUG
#endif

/* undo some macro defs in pyport.h */
#if defined(_PY_PORT_CTYPE_UTF8_ISSUE) && defined(__cplusplus)
#undef isalnum
#undef isalpha
#undef islower
#undef isspace
#undef isupper
#undef tolower
#undef toupper
#endif

/* This logic is borrowed from mpi4py/vtkmpi4py/src/atimport.h */
#ifdef VTK_NO_PYTHON_THREADS
#undef  PyGILState_Ensure
#define PyGILState_Ensure() ((PyGILState_STATE)0)
#undef  PyGILState_Release
#define PyGILState_Release(state) (state)=((PyGILState_STATE)0)
#endif

// Description:
// RAII class to manage Python threading using GIL (Global Interpreter Lock).
// GIL is locked at object creation and unlocked at destruction.
// Note: behaviour of this class depends on VTK_PYTHON_FULL_THREADSAFE.
class vtkPythonScopeGilEnsurer 
{
public:
  // Description:
  // If force is TRUE, lock/unlock even if VTK_PYTHON_FULL_THREADSAFE is not defined.
  // If force is FALSE, lock/unlock is only performed if VTK_PYTHON_FULL_THREADSAFE is
  // defined.
  // If noRelease is TRUE, unlock will not be called at object destruction. This is used
  // for specific python function calls like PyFinalize which already take 
  // care of releasing the GIL.
  vtkPythonScopeGilEnsurer(bool force = false, bool noRelease = false)
    : State(PyGILState_UNLOCKED)
  {
#ifdef VTK_PYTHON_FULL_THREADSAFE
    // Force is always true with VTK_PYTHON_FULL_THREADSAFE
    force = true;
#endif
    this->Force = force;
    this->NoRelease = noRelease;
    if (this->Force)
      {
      this->State = PyGILState_Ensure();
      }
  }
 
  ~vtkPythonScopeGilEnsurer()
  {
    if (this->Force && !this->NoRelease)
      {
      PyGILState_Release(this->State);
      }
  }

private:
  PyGILState_STATE State;
  bool Force;
  bool NoRelease;

  vtkPythonScopeGilEnsurer(const vtkPythonScopeGilEnsurer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPythonScopeGilEnsurer&) VTK_DELETE_FUNCTION;
};


#endif
