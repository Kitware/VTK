/*
 * tkWinPort.h --
 *
 *	This header file handles porting issues that occur because of
 *	differences between Windows and Unix. It should be the only
 *	file that contains #ifdefs to handle different flavors of OS.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tkWinPort.h 1.12 96/10/02 15:40:59
 */

#ifndef _WINPORT
#define _WINPORT

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <malloc.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _MSC_VER
#    define strncasecmp strnicmp
#    define hypot _hypot
#else /* _MSC_VER */
#    define strncasecmp strncmpi
#endif /* _MSC_VER */

#define NBBY 8

#define OPEN_MAX 32

/*
 * The following define causes Tk to use its internal keysym hash table
 */

#define REDO_KEYSYM_LOOKUP

/*
 * The following macro checks to see whether there is buffered
 * input data available for a stdio FILE.
 */

#ifdef _MSC_VER
#    define TK_READ_DATA_PENDING(f) ((f)->_cnt > 0)
#else /* _MSC_VER */
#    define TK_READ_DATA_PENDING(f) ((f)->level > 0)
#endif /* _MSC_VER */

/*
 * The following stubs implement various calls that don't do anything
 * under Windows.
 */

#define TkFreeWindowId(dispPtr,w)
#define TkInitXId(dispPtr)
#define XFlush(display)
#define XGrabServer(display)
#define XUngrabServer(display)

/*
 * The following X functions are implemented as macros under Windows.
 */

#define XFree(data) {if ((data) != NULL) ckfree((char *) (data));}
#define XNoOp(display) {display->request++;}
#define XSynchronize(display, bool) {display->request++;}
#define XSync(display, bool) {display->request++;}
#define XVisualIDFromVisual(visual) (visual->visualid)

/*
 * The following Tk functions are implemented as macros under Windows.
 */

#define TkGetNativeProlog(interp) TkGetProlog(interp)

/*
 * Define timezone for gettimeofday.
 */

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

extern int gettimeofday(struct timeval *, struct timezone *);
extern void panic();

#endif /* _WINPORT */
