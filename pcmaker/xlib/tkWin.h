/*
 * tkWin.h --
 *
 *	Declarations of public types and interfaces that are only
 *	available under Windows.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tkWin.h 1.6 96/08/15 13:19:41
 */

#ifndef _TKWIN
#define _TKWIN

#ifndef _TK
#include <tk.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

/*
 * The following messages are use to communicate between a Tk toplevel
 * and its container window.
 */

#define TK_CLAIMFOCUS	(WM_USER)
#define TK_GEOMETRYREQ	(WM_USER+1)
#define TK_ATTACHWINDOW	(WM_USER+2)
#define TK_DETACHWINDOW	(WM_USER+3)


/*
 *--------------------------------------------------------------
 *
 * Exported procedures defined for the Windows platform only.
 *
 *--------------------------------------------------------------
 */

EXTERN Window		Tk_AttachHWND _ANSI_ARGS_((Tk_Window tkwin,
			    HWND hwnd));
EXTERN HINSTANCE 	Tk_GetHINSTANCE _ANSI_ARGS_((void));
EXTERN HWND		Tk_GetHWND _ANSI_ARGS_((Window window));
EXTERN Tk_Window	Tk_HWNDToWindow _ANSI_ARGS_((HWND hwnd));
EXTERN void		Tk_PointerEvent _ANSI_ARGS_((HWND hwnd,
			    int x, int y));
EXTERN int		Tk_TranslateWinEvent _ANSI_ARGS_((HWND hwnd,
			    UINT message, WPARAM wParam, LPARAM lParam,
			    LRESULT *result));

#endif /* _TKWIN */
