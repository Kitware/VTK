/*
 * tkMacOSXInt.h --
 *
 *  Declarations of Macintosh specific shared variables and procedures.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 * Copyright (c) 2005-2007 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TKMACINT
#define _TKMACINT

#ifndef _TKINT
#include "tkInt.h"
#endif

#define TextStyle MacTextStyle
#include <Carbon/Carbon.h>
#undef TextStyle

/*
 * Include platform specific public interfaces.
 */

#ifndef _TKMAC
#include "tkMacOSX.h"
#endif

struct TkWindowPrivate {
    TkWindow *winPtr;    /* Ptr to tk window or NULL if Pixmap */
    CGrafPtr grafPtr;
    CGContextRef context;
    ControlRef rootControl;
    int xOff;      /* X offset from toplevel window */
    int yOff;      /* Y offset from toplevel window */
    CGSize size;
    HIShapeRef visRgn;    /* Visible region of window */
    HIShapeRef aboveVisRgn;  /* Visible region of window & its children */
    CGRect drawRect;    /* Clipped drawing rect */
    int referenceCount;    /* Don't delete toplevel until children are
         * gone. */
    struct TkWindowPrivate *toplevel;
        /* Pointer to the toplevel datastruct. */
    int flags;      /* Various state see defines below. */
};
typedef struct TkWindowPrivate MacDrawable;

/*
 * This list is used to keep track of toplevel windows that have a Mac
 * window attached. This is useful for several things, not the least
 * of which is maintaining floating windows.
 */

typedef struct TkMacOSXWindowList {
    struct TkMacOSXWindowList *nextPtr;
        /* The next window in the list. */
    TkWindow *winPtr;    /* This window */
} TkMacOSXWindowList;

/*
 * Defines use for the flags field of the MacDrawable data structure.
 */

#define TK_SCROLLBAR_GROW  0x01
#define TK_CLIP_INVALID    0x02
#define TK_HOST_EXISTS    0x04
#define TK_DRAWN_UNDER_MENU  0x08
#define TK_CLIPPED_DRAW    0x10
#define TK_IS_PIXMAP    0x20
#define TK_IS_BW_PIXMAP    0x40

/*
 * I am reserving TK_EMBEDDED = 0x100 in the MacDrawable flags
 * This is defined in tk.h. We need to duplicate the TK_EMBEDDED flag in the
 * TkWindow structure for the window, but in the MacWin. This way we can
 * still tell what the correct port is after the TKWindow structure has been
 * freed. This actually happens when you bind destroy of a toplevel to
 * Destroy of a child.
 */

/*
 * This structure is for handling Netscape-type in process
 * embedding where Tk does not control the top-level. It contains
 * various functions that are needed by Mac specific routines, like
 * TkMacOSXGetDrawablePort. The definitions of the function types
 * are in tkMacOSX.h.
 */

typedef struct {
    Tk_MacOSXEmbedRegisterWinProc *registerWinProc;
    Tk_MacOSXEmbedGetGrafPortProc *getPortProc;
    Tk_MacOSXEmbedMakeContainerExistProc *containerExistProc;
    Tk_MacOSXEmbedGetClipProc *getClipProc;
    Tk_MacOSXEmbedGetOffsetInParentProc *getOffsetProc;
} TkMacOSXEmbedHandler;

MODULE_SCOPE TkMacOSXEmbedHandler *tkMacOSXEmbedHandler;

/*
 * Defines used for TkMacOSXInvalidateWindow
 */

#define TK_WINDOW_ONLY 0
#define TK_PARENT_WINDOW 1

/*
 * Accessor for the privatePtr flags field for the TK_HOST_EXISTS field
 */

#define TkMacOSXHostToplevelExists(tkwin) \
    (((TkWindow *) (tkwin))->privatePtr->toplevel->flags & TK_HOST_EXISTS)

/*
 * Defines use for the flags argument to TkGenWMConfigureEvent.
 */

#define TK_LOCATION_CHANGED  1
#define TK_SIZE_CHANGED    2
#define TK_BOTH_CHANGED    3

/*
 * Defines for tkTextDisp.c
 */

#define TK_LAYOUT_WITH_BASE_CHUNKS  1
#define TK_DRAW_IN_CONTEXT    1

#if !TK_DRAW_IN_CONTEXT
MODULE_SCOPE int TkMacOSXCompareColors(unsigned long c1, unsigned long c2);
#endif

/*
 * Globals shared among TkAqua.
 */

MODULE_SCOPE MenuHandle tkCurrentAppleMenu; /* Handle to current Apple Menu */
MODULE_SCOPE MenuHandle tkAppleMenu;  /* Handle to default Apple Menu */
MODULE_SCOPE MenuHandle tkFileMenu;  /* Handles to menus */
MODULE_SCOPE MenuHandle tkEditMenu;  /* Handles to menus */
MODULE_SCOPE int tkPictureIsOpen;  /* If this is 1, we are drawing to a
           * picture The clipping should then be
           * done relative to the bounds of the
           * picture rather than the window. As
           * of OS X.0.4, something is seriously
           * wrong: The clipping bounds only
           * seem to work if the top,left values
           * are 0,0 The destination rectangle
           * for CopyBits should also have
           * top,left values of 0,0
           */
MODULE_SCOPE TkMacOSXWindowList *tkMacOSXWindowListPtr; /* List of toplevels */
MODULE_SCOPE Tcl_Encoding TkMacOSXCarbonEncoding;

/*
 * Prototypes of internal procs not in the stubs table.
 */

MODULE_SCOPE void TkMacOSXDefaultStartupScript(void);
#if 0
MODULE_SCOPE int XSetClipRectangles(Display *d, GC gc, int clip_x_origin,
  int clip_y_origin, XRectangle* rectangles, int n, int ordering);
#endif
MODULE_SCOPE void TkpClipDrawableToRect(Display *display, Drawable d, int x,
  int y, int width, int height);
MODULE_SCOPE void TkpRetainRegion(TkRegion r);
MODULE_SCOPE void TkpReleaseRegion(TkRegion r);

/*
 * Include the stubbed internal platform-specific API.
 */

#include "tkIntPlatDecls.h"

#endif /* _TKMACINT */
