/*
 * tkWinInt.h --
 *
 *  This file contains declarations that are shared among the
 *  Windows-specific parts of Tk, but aren't used by the rest of
 *  Tk.
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
#define WS_EX_TOOLWINDOW  0x00000080L 
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

#define TWD_BITMAP  1
#define TWD_WINDOW  2
#define TWD_WINDC  3

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

#define TkWinGetHWND(w)    (((TkWinDrawable *) w)->window.handle)
#define TkWinGetWinPtr(w)  (((TkWinDrawable *) w)->window.winPtr)
#define TkWinGetHBITMAP(w)  (((TkWinDrawable *) w)->bitmap.handle)
#define TkWinGetColormap(w)  (((TkWinDrawable *) w)->bitmap.colormap)
#define TkWinGetHDC(w)    (((TkWinDrawable *) w)->winDC.hdc)

/*
 * The following structure is used to encapsulate palette information.
 */

typedef struct {
    HPALETTE palette;    /* Palette handle used when drawing. */
    UINT size;      /* Number of entries in the palette. */
    int stale;      /* 1 if palette needs to be realized,
         * otherwise 0.  If the palette is stale,
         * then an idle handler is scheduled to
         * realize the palette. */
    Tcl_HashTable refCounts;  /* Hash table of palette entry reference counts
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
#define TK_WIN_TOPLEVEL_NOCDC_CLASS_NAME "TkTopLevelNoCDC"
#define TK_WIN_CHILD_CLASS_NAME "TkChild"

/*
 * The following variable is a translation table between X gc functions and
 * Win32 raster and BitBlt op modes.
 */

extern int tkpWinRopModes[];
extern int tkpWinBltModes[];

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

EXTERN LRESULT CALLBACK  TkWinChildProc _ANSI_ARGS_((HWND hwnd, UINT message,
          WPARAM wParam, LPARAM lParam));

/*
 * Special proc needed as tsd accessor function between
 * tkWinX.c:GenerateXEvent and tkWinClipboard.c:UpdateClipboard
 */
EXTERN void  TkWinUpdatingClipboard(int mode);

/*
 * Used by tkWinDialog.c to associate the right icon with tk_messageBox
 */
EXTERN HICON  TkWinGetIcon(Tk_Window tkw, DWORD iconsize);

/*
 * Used by tkWinX.c on for certain system display change messages
 */
EXTERN void  TkWinDisplayChanged(Display *display);

/*
 * The following structure keeps track of whether we are using the 
 * multi-byte or the wide-character interfaces to the operating system.
 * System calls should be made through the following function table.
 *
 * While some system calls need to use this A/W jump-table, it is not
 * necessary for all calls to do it, which is why you won't see this
 * used throughout the Tk code, but only in key areas. -- hobbs
 */

typedef struct TkWinProcs {
    int useWide;
    LRESULT (WINAPI *callWindowProc)(WNDPROC lpPrevWndFunc, HWND hWnd,
      UINT Msg, WPARAM wParam, LPARAM lParam);
    LRESULT (WINAPI *defWindowProc)(HWND hWnd, UINT Msg, WPARAM wParam,
      LPARAM lParam);
    ATOM (WINAPI *registerClass)(CONST WNDCLASS *lpWndClass);
    BOOL (WINAPI *setWindowText)(HWND hWnd, LPCTSTR lpString);
    HWND (WINAPI *createWindowEx)(DWORD dwExStyle, LPCTSTR lpClassName,
      LPCTSTR lpWindowName, DWORD dwStyle, int x, int y,
      int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
      HINSTANCE hInstance, LPVOID lpParam);
    BOOL (WINAPI *insertMenu)(HMENU hMenu, UINT uPosition, UINT uFlags,
      UINT uIDNewItem, LPCTSTR lpNewItem);
} TkWinProcs;

EXTERN TkWinProcs *tkWinProcs;

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

/*
 * The following allows us to cache these encoding for multiple functions.
 */


extern Tcl_Encoding TkWinGetKeyInputEncoding _ANSI_ARGS_((void));
extern Tcl_Encoding TkWinGetUnicodeEncoding _ANSI_ARGS_((void));

/*
 * Values returned by TkWinGetPlatformTheme.
 */
#define TK_THEME_WIN_CLASSIC    1
#define TK_THEME_WIN_XP         2

#endif /* _TKWININT */

