// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(VTK_STD_AFX_H)
#define VTK_STD_AFX_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN    // Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>    // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>      // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "vtkConfigure.h"

#if defined(VTK_BUILD_SHARED_LIBS)
#  if defined(vtkMFC_EXPORTS)
#    define VTK_MFC_EXPORT __declspec( dllexport )
#  else
#    define VTK_MFC_EXPORT __declspec( dllimport )
#  endif
#else
#  define VTK_MFC_EXPORT
#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(VTK_STD_AFX_H)
