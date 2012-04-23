/*
 * tkMacOSXInt.h --
 *
 *  Declarations of Macintosh specific shared variables and procedures.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
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
    TkWindow *winPtr;       /* Ptr to tk window or NULL if Pixmap */
    CGrafPtr  grafPtr;
    ControlRef rootControl;
    int xOff;             /* X offset from toplevel window */
    int yOff;             /* Y offset from toplevel window */
    RgnHandle clipRgn;    /* Visable region of window */
    RgnHandle aboveClipRgn;  /* Visable region of window & it's children */
    int referenceCount;    /* Don't delete toplevel until children are
         * gone. */
    struct TkWindowPrivate *toplevel;  /* Pointer to the toplevel
           * datastruct. */
    int flags;      /* Various state see defines below. */
};
typedef struct TkWindowPrivate MacDrawable;

/*
 * This list is used to keep track of toplevel windows that have a Mac
 * window attached. This is useful for several things, not the least
 * of which is maintaining floating windows.
 */

typedef struct TkMacOSXWindowList {
    struct TkMacOSXWindowList *nextPtr;  /* The next window in the list. */
    TkWindow *winPtr;      /* This window */
} TkMacOSXWindowList;

/*
 * Defines use for the flags field of the MacDrawable data structure.
 */
 
#define TK_SCROLLBAR_GROW  1
#define TK_CLIP_INVALID    2
#define TK_HOST_EXISTS    4
#define TK_DRAWN_UNDER_MENU  8
#define TK_MAPPED_IN_PARENT  16

/*
 * I am reserving TK_EMBEDDED = 0x100 in the MacDrawable flags
 * This is defined in tk.h. We need to duplicate the TK_EMBEDDED flag in the
 * TkWindow structure for the window, but in the MacWin.  This way we can
 * still tell what the correct port is after the TKWindow structure has been
 * freed.  This actually happens when you bind destroy of a toplevel to
 * Destroy of a child.
 */

/*
 * This structure is for handling Netscape-type in process
 * embedding where Tk does not control the top-level.  It contains
 * various functions that are needed by Mac specific routines, like
 * TkMacOSXGetDrawablePort.  The definitions of the function types
 * are in tclMac.h.
 */

typedef struct {
  Tk_MacOSXEmbedRegisterWinProc *registerWinProc;
  Tk_MacOSXEmbedGetGrafPortProc *getPortProc;
  Tk_MacOSXEmbedMakeContainerExistProc *containerExistProc;
  Tk_MacOSXEmbedGetClipProc *getClipProc;
  Tk_MacOSXEmbedGetOffsetInParentProc *getOffsetProc;
} TkMacOSXEmbedHandler;

extern TkMacOSXEmbedHandler *gMacEmbedHandler;

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
 * Variables shared among various Mac Tk modules but are not
 * exported to the outside world.
 */

/*
 * Globals shared among Macintosh Tk
 */

extern MenuHandle tkAppleMenu;    /* Handle to the Apple Menu */
extern MenuHandle tkFileMenu;    /* Handles to menus */
extern MenuHandle tkEditMenu;    /* Handles to menus */
extern RgnHandle tkMenuCascadeRgn;  /* A region to clip with. */
extern int tkUseMenuCascadeRgn;    /* If this is 1, clipping code
           * should intersect tkMenuCascadeRgn
           * before drawing occurs.
           * tkMenuCascadeRgn will only
           * be valid when the value of this
           * variable is 1. */
extern int tkPictureIsOpen;    /* If this is 1, we are drawing to a
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
extern TkMacOSXWindowList *tkMacOSXWindowListPtr;
          /* The list of toplevels */

extern Tcl_Encoding TkMacOSXCarbonEncoding;

extern void TkMacOSXDisplayChanged(Display *display);
extern int TkMacOSXUseAntialiasedText(Tcl_Interp *interp, int enable);
extern void TkMacOSXInitCarbonEvents(Tcl_Interp *interp);
extern int TkMacOSXInitCGDrawing(Tcl_Interp *interp, int enable, int antiAlias);
extern void TkMacOSXDefaultStartupScript(void);
extern int TkMacOSXGenerateFocusEvent( Window window, int activeFlag);
extern WindowClass TkMacOSXWindowClass(TkWindow *winPtr);
extern int TkMacOSXIsWindowZoomed(TkWindow *winPtr);

extern void* TkMacOSXGetNamedSymbol(const char* module, const char* symbol);
/* Macro to abstract common use of TkMacOSXGetNamedSymbol to initialize named symbols */
#define TkMacOSXInitNamedSymbol(module, ret, symbol, ...) \
    static ret (* symbol)(__VA_ARGS__) = (void*)(-1L); \
    if (symbol == (void*)(-1L)) { \
        symbol = TkMacOSXGetNamedSymbol(STRINGIFY(module), STRINGIFY(_##symbol));\
    }

#include "tkIntPlatDecls.h"

#endif /* _TKMACINT */
