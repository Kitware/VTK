/*
 * tkMacOSXPort.h --
 *
 *  This file is included by all of the Tk C files.  It contains
 *  information that may be configuration-dependent, such as
 *  #includes for system include files and a few other things.
 *
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TKMACPORT
#define _TKMACPORT

/*
 * Macro to use instead of "void" for arguments that must have
 * type "void *" in ANSI C;  maps them to type "char *" in
 * non-ANSI systems.  This macro may be used in some of the include
 * files below, which is why it is defined here.
 */

#ifndef VOID
#   ifdef __STDC__
#       define VOID void
#   else
#       define VOID char
#   endif
#endif

#ifndef _TCL
#   include <tcl.h>
#endif

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "tclMath.h"
#include <ctype.h>
#include <limits.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xfuncproto.h>
#include <X11/Xutil.h>
#include "tkIntXlibDecls.h"

/*
 * Not all systems declare the errno variable in errno.h. so this
 * file does it explicitly.
 */

extern int errno;

/*
 * Define "NBBY" (number of bits per byte) if it's not already defined.
 */

#ifndef NBBY
#   define NBBY 8
#endif

/*
 * Declarations for various library procedures that may not be declared
 * in any other header file.
 */

#ifndef panic  /* In a stubs-aware setting, this could confuse the #define */
extern void     panic  _ANSI_ARGS_(TCL_VARARGS(char *, string));
#endif
#ifndef strcasecmp
extern int    strcasecmp _ANSI_ARGS_((CONST char *s1,
          CONST char *s2));
#endif
#ifndef strncasecmp          
extern int    strncasecmp _ANSI_ARGS_((CONST char *s1,
          CONST char *s2, size_t n));
#endif
/*
 * Defines for X functions that are used by Tk but are treated as
 * no-op functions on the Macintosh.
 */

#define XFlush(display)
#define XFree(data) {if ((data) != NULL) ckfree((char *) (data));}
#define XGrabServer(display)
#define XNoOp(display) {display->request++;}
#define XUngrabServer(display)
#define XSynchronize(display, bool) {display->request++;}
#define XSync(display, bool) {display->request++;}
#define XVisualIDFromVisual(visual) (visual->visualid)

/*
 * The following functions are not used on the Mac, so we stub them out.
 */

#define TkFreeWindowId(dispPtr,w)
#define TkInitXId(dispPtr)
#define TkpButtonSetDefaults(specPtr) {}
#define TkpCmapStressed(tkwin,colormap) (0)
#define TkpFreeColor(tkColPtr)
#define TkSetPixmapColormap(p,c) {}
#define TkpSync(display)

/*
 * The following macro returns the pixel value that corresponds to the
 * RGB values in the given XColor structure.
 */

#define PIXEL_MAGIC ((unsigned char) 0x69)
#define TkpGetPixel(p) ((((((PIXEL_MAGIC << 8) \
  | (((p)->red >> 8) & 0xff)) << 8) \
  | (((p)->green >> 8) & 0xff)) << 8) \
  | (((p)->blue >> 8) & 0xff))

/*
 * This macro stores a representation of the window handle in a string.
 * This should perhaps use the real size of an XID.
 */

#define TkpPrintWindowId(buf,w) \
  sprintf((buf), "0x%x", (unsigned int) (w))
      
/*
 * TkpScanWindowId is just an alias for Tcl_GetInt on Unix.
 */

#define TkpScanWindowId(i,s,wp) \
  Tcl_GetInt((i),(s),(int *) (wp))

/*
 * Magic pixel values for dynamic (or active) colors.
 */

#define HIGHLIGHT_PIXEL      31
#define HIGHLIGHT_TEXT_PIXEL    33
#define CONTROL_TEXT_PIXEL    35
#define CONTROL_BODY_PIXEL    37
#define CONTROL_FRAME_PIXEL    39
#define WINDOW_BODY_PIXEL    41
#define MENU_ACTIVE_PIXEL    43
#define MENU_ACTIVE_TEXT_PIXEL    45
#define MENU_BACKGROUND_PIXEL    47
#define MENU_DISABLED_PIXEL    49
#define MENU_TEXT_PIXEL      51
#define APPEARANCE_PIXEL    52

#endif /* _TKMACPORT */

