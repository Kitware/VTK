/*
 * tkWinInt.h --
 *
 *      This file contains declarations that are shared among the
 *      Windows-specific parts of Tk, but aren't used by the rest of
 *      Tk.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TKWININT
#define _TKWININT

#ifndef _TKINT
#include "tkInt.h"
#endif

/*
 * Include platform specific public interfaces.
 */

#ifndef _TKWIN
#include "tkWin.h"
#endif

#ifndef _TKPORT
#include "tkPort.h"
#endif


/*
 * Define constants missing from older Win32 SDK header files.
 */

#ifndef WS_EX_TOOLWINDOW
#define WS_EX_TOOLWINDOW        0x00000080L 
#endif

/*
 * The TkWinDCState is used to save the state of a device context
 * so that it can be restored later.
 */

typedef struct TkWinDCState {
    HPALETTE palette;
    int bkmode;
} TkWinDCState;

/*
 * The TkWinDrawable is the internal implementation of an X Drawable (either
 * a Window or a Pixmap).  The following constants define the valid Drawable
 * types.
 */

#define TWD_BITMAP      1
#define TWD_WINDOW      2
#define TWD_WINDC       3

typedef struct {
    int type;
    HWND handle;
    TkWindow *winPtr;
} TkWinWindow;

typedef struct {
    int type;
    HBITMAP handle;
    Colormap colormap;
    int depth;
} TkWinBitmap;

typedef struct {
    int type;
    HDC hdc;
}TkWinDC;

typedef union {
    int type;
    TkWinWindow window;
    TkWinBitmap bitmap;
    TkWinDC winDC;
} TkWinDrawable;

/*
 * The following macros are used to retrieve internal values from a Drawable.
 */

#define TkWinGetHWND(w)         (((TkWinDrawable *) w)->window.handle)
#define TkWinGetWinPtr(w)       (((TkWinDrawable *) w)->window.winPtr)
#define TkWinGetHBITMAP(w)      (((TkWinDrawable *) w)->bitmap.handle)
#define TkWinGetColormap(w)     (((TkWinDrawable *) w)->bitmap.colormap)
#define TkWinGetHDC(w)          (((TkWinDrawable *) w)->winDC.hdc)

/*
 * The following structure is used to encapsulate palette information.
 */

typedef struct {
    HPALETTE palette;           /* Palette handle used when drawing. */
    UINT size;                  /* Number of entries in the palette. */
    int stale;                  /* 1 if palette needs to be realized,
                                 * otherwise 0.  If the palette is stale,
                                 * then an idle handler is scheduled to
                                 * realize the palette. */
    Tcl_HashTable refCounts;    /* Hash table of palette entry reference counts
                                 * indexed by pixel value. */
} TkWinColormap;

/*
 * The following macro retrieves the Win32 palette from a colormap.
 */

#define TkWinGetPalette(colormap) (((TkWinColormap *) colormap)->palette)

/*
 * The following macros define the class names for Tk Window types.
 */

#define TK_WIN_TOPLEVEL_CLASS_NAME "TkTopLevel"
#define TK_WIN_CHILD_CLASS_NAME "TkChild"

/*
 * The following variable is a translation table between X gc functions and
 * Win32 raster op modes.
 */

extern int tkpWinRopModes[];

/*
 * The following defines are used with TkWinGetBorderPixels to get the
 * extra 2 border colors from a Tk_3DBorder.
 */

#define TK_3D_LIGHT2 TK_3D_DARK_GC+1
#define TK_3D_DARK2 TK_3D_DARK_GC+2

/*
 * Internal procedures used by more than one source file.
 */

#include "tkIntPlatDecls.h"

/*
 * We need to specially add the TkWinChildProc because of the special
 * prototype it has (doesn't fit into stubs schema)
 */
#ifdef BUILD_tk
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif

EXTERN LRESULT CALLBACK TkWinChildProc _ANSI_ARGS_((HWND hwnd, UINT message,
                            WPARAM wParam, LPARAM lParam));

/*
 * Special proc needed as tsd accessor function between
 * tkWinX.c:GenerateXEvent and tkWinClipboard.c:UpdateClipboard
 */
EXTERN void     TkWinUpdatingClipboard(int mode);

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _TKWININT */

