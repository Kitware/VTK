/*
 * tkWin.h --
 *
 *  Declarations of public types and interfaces that are only
 *  available under Windows.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TKWIN
#define _TKWIN

#ifndef _TK
#include <tk.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#ifdef BUILD_tk
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * The following messages are use to communicate between a Tk toplevel
 * and its container window.
 */

#define TK_CLAIMFOCUS  (WM_USER)
#define TK_GEOMETRYREQ  (WM_USER+1)
#define TK_ATTACHWINDOW  (WM_USER+2)
#define TK_DETACHWINDOW  (WM_USER+3)


/*
 *--------------------------------------------------------------
 *
 * Exported procedures defined for the Windows platform only.
 *
 *--------------------------------------------------------------
 */

#include "tkPlatDecls.h"

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _TKWIN */
