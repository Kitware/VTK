/*
 * tkInt.h --
 *
 *      Declarations for things used internally by the Tk
 *      procedures but not exported outside the module.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: Id 
 */

#ifndef _TKINT
#define _TKINT

#ifndef _TK
#include "tk.h"
#endif
#ifndef _TCL
#include "tcl.h"
#endif
#ifndef _TKPORT
#include <tkPort.h>
#endif

/*
 * Opaque type declarations:
 */

typedef struct TkColormap TkColormap;
typedef struct TkGrabEvent TkGrabEvent;
typedef struct TkpCursor_ *TkpCursor;
typedef struct TkRegion_ *TkRegion;
typedef struct TkStressedCmap TkStressedCmap;
typedef struct TkBindInfo_ *TkBindInfo;

/*
 * Procedure types.
 */

typedef int (TkBindEvalProc) _ANSI_ARGS_((ClientData clientData,
        Tcl_Interp *interp, XEvent *eventPtr, Tk_Window tkwin,
        KeySym keySym));
typedef void (TkBindFreeProc) _ANSI_ARGS_((ClientData clientData));
typedef Window (TkClassCreateProc) _ANSI_ARGS_((Tk_Window tkwin,
        Window parent, ClientData instanceData));
typedef void (TkClassGeometryProc) _ANSI_ARGS_((ClientData instanceData));
typedef void (TkClassModalProc) _ANSI_ARGS_((Tk_Window tkwin,
        XEvent *eventPtr));


/*
 * Widget class procedures used to implement platform specific widget
 * behavior.
 */

typedef struct TkClassProcs {
    TkClassCreateProc *createProc;
                                /* Procedure to invoke when the
                                   platform-dependent window needs to be
                                   created. */
    TkClassGeometryProc *geometryProc;
                                /* Procedure to invoke when the geometry of a
                                   window needs to be recalculated as a result
                                   of some change in the system. */
    TkClassModalProc *modalProc;
                                /* Procedure to invoke after all bindings on a
                                   widget have been triggered in order to
                                   handle a modal loop. */
} TkClassProcs;

/*
 * One of the following structures is maintained for each cursor in
 * use in the system.  This structure is used by tkCursor.c and the
 * various system specific cursor files.
 */

typedef struct TkCursor {
    Tk_Cursor cursor;           /* System specific identifier for cursor. */
    Display *display;           /* Display containing cursor. Needed for
                                 * disposal and retrieval of cursors. */
    int resourceRefCount;       /* Number of active uses of this cursor (each
                                 * active use corresponds to a call to
                                 * Tk_AllocPreserveFromObj or Tk_Preserve).
                                 * If this count is 0, then this structure
                                 * is no longer valid and it isn't present
                                 * in a hash table: it is being kept around
                                 * only because there are objects referring
                                 * to it.  The structure is freed when
                                 * resourceRefCount and objRefCount are
                                 * both 0. */
    int objRefCount;            /* Number of Tcl objects that reference
                                 * this structure.. */
    Tcl_HashTable *otherTable;  /* Second table (other than idTable) used
                                 * to index this entry. */
    Tcl_HashEntry *hashPtr;     /* Entry in otherTable for this structure
                                 * (needed when deleting). */
    Tcl_HashEntry *idHashPtr;   /* Entry in idTable for this structure
                                 * (needed when deleting). */
    struct TkCursor *nextPtr;   /* Points to the next TkCursor structure with
                                 * the same name.  Cursors with the same
                                 * name but different displays are chained
                                 * together off a single hash table entry. */
} TkCursor;

/*
 * One of the following structures is maintained for each display
 * containing a window managed by Tk.  In part, the structure is 
 * used to store thread-specific data, since each thread will have 
 * its own TkDisplay structure.
 */

typedef struct TkDisplay {
    Display *display;           /* Xlib's info about display. */
    struct TkDisplay *nextPtr;  /* Next in list of all displays. */
    char *name;                 /* Name of display (with any screen
                                 * identifier removed).  Malloc-ed. */
    Time lastEventTime;         /* Time of last event received for this
                                 * display. */

    /*
     * Information used primarily by tk3d.c:
     */

    int borderInit;             /* 0 means borderTable needs initializing. */
    Tcl_HashTable borderTable;  /* Maps from color name to TkBorder 
                                 * structure. */

    /*
     * Information used by tkAtom.c only:
     */

    int atomInit;               /* 0 means stuff below hasn't been
                                 * initialized yet. */
    Tcl_HashTable nameTable;    /* Maps from names to Atom's. */
    Tcl_HashTable atomTable;    /* Maps from Atom's back to names. */

    /*
     * Information used primarily by tkBind.c:
     */

    int bindInfoStale;          /* Non-zero means the variables in this
                                 * part of the structure are potentially
                                 * incorrect and should be recomputed. */
    unsigned int modeModMask;   /* Has one bit set to indicate the modifier
                                 * corresponding to "mode shift".  If no
                                 * such modifier, than this is zero. */
    unsigned int metaModMask;   /* Has one bit set to indicate the modifier
                                 * corresponding to the "Meta" key.  If no
                                 * such modifier, then this is zero. */
    unsigned int altModMask;    /* Has one bit set to indicate the modifier
                                 * corresponding to the "Meta" key.  If no
                                 * such modifier, then this is zero. */
    enum {LU_IGNORE, LU_CAPS, LU_SHIFT} lockUsage;
                                /* Indicates how to interpret lock modifier. */
    int numModKeyCodes;         /* Number of entries in modKeyCodes array
                                 * below. */
    KeyCode *modKeyCodes;       /* Pointer to an array giving keycodes for
                                 * all of the keys that have modifiers
                                 * associated with them.  Malloc'ed, but
                                 * may be NULL. */

    /*
     * Information used by tkBitmap.c only:
     */
  
    int bitmapInit;             /* 0 means tables above need initializing. */
    int bitmapAutoNumber;       /* Used to number bitmaps. */
    Tcl_HashTable bitmapNameTable;    
                                /* Maps from name of bitmap to the first 
                                 * TkBitmap record for that name. */
    Tcl_HashTable bitmapIdTable;/* Maps from bitmap id to the TkBitmap
                                 * structure for the bitmap. */
    Tcl_HashTable bitmapDataTable;    
                                /* Used by Tk_GetBitmapFromData to map from
                                 * a collection of in-core data about a 
                                 * bitmap to a reference giving an auto-
                                 * matically-generated name for the bitmap. */

    /*
     * Information used by tkCanvas.c only:
     */

    int numIdSearches;          
    int numSlowSearches;

    /*
     * Used by tkColor.c only:
     */

    int colorInit;              /* 0 means color module needs initializing. */
    TkStressedCmap *stressPtr;  /* First in list of colormaps that have
                                 * filled up, so we have to pick an
                                 * approximate color. */
    Tcl_HashTable colorNameTable;
                                /* Maps from color name to TkColor structure
                                 * for that color. */
    Tcl_HashTable colorValueTable;
                                /* Maps from integer RGB values to TkColor
                                 * structures. */

    /*
     * Used by tkCursor.c only:
     */

    int cursorInit;             /* 0 means cursor module need initializing. */
    Tcl_HashTable cursorNameTable;
                                /* Maps from a string name to a cursor to the
                                 * TkCursor record for the cursor. */
    Tcl_HashTable cursorDataTable;
                                /* Maps from a collection of in-core data
                                 * about a cursor to a TkCursor structure. */
    Tcl_HashTable cursorIdTable;
                                /* Maps from a cursor id to the TkCursor
                                 * structure for the cursor. */
    char cursorString[20];      /* Used to store a cursor id string. */
    Font cursorFont;            /* Font to use for standard cursors.
                                 * None means font not loaded yet. */

    /*
     * Information used by tkError.c only:
     */

    struct TkErrorHandler *errorPtr;
                                /* First in list of error handlers
                                 * for this display.  NULL means
                                 * no handlers exist at present. */
    int deleteCount;            /* Counts # of handlers deleted since
                                 * last time inactive handlers were
                                 * garbage-collected.  When this number
                                 * gets big, handlers get cleaned up. */

    /*
     * Used by tkEvent.c only:
     */

    struct TkWindowEvent *delayedMotionPtr;
                                /* Points to a malloc-ed motion event
                                 * whose processing has been delayed in
                                 * the hopes that another motion event
                                 * will come along right away and we can
                                 * merge the two of them together.  NULL
                                 * means that there is no delayed motion
                                 * event. */

    /*
     * Information used by tkFocus.c only:
     */

    int focusDebug;             /* 1 means collect focus debugging 
                                 * statistics. */
    struct TkWindow *implicitWinPtr;
                                /* If the focus arrived at a toplevel window
                                 * implicitly via an Enter event (rather
                                 * than via a FocusIn event), this points
                                 * to the toplevel window.  Otherwise it is
                                 * NULL. */
    struct TkWindow *focusPtr;  /* Points to the window on this display that
                                 * should be receiving keyboard events.  When
                                 * multiple applications on the display have
                                 * the focus, this will refer to the
                                 * innermost window in the innermost
                                 * application.  This information isn't used
                                 * under Unix or Windows, but it's needed on
                                 * the Macintosh. */

    /*
     * Information used by tkGC.c only:
     */
    
    Tcl_HashTable gcValueTable; /* Maps from a GC's values to a TkGC structure
                                 * describing a GC with those values. */
    Tcl_HashTable gcIdTable;    /* Maps from a GC to a TkGC. */ 
    int gcInit;                 /* 0 means the tables below need 
                                 * initializing. */

    /*
     * Information used by tkGeometry.c only:
     */

    Tcl_HashTable maintainHashTable;
                                /* Hash table that maps from a master's 
                                 * Tk_Window token to a list of slaves
                                 * managed by that master. */
    int geomInit;    

    /*
     * Information used by tkGet.c only:
     */
  
    Tcl_HashTable uidTable;     /* Stores all Tk_Uid  used in a thread. */
    int uidInit;                /* 0 means uidTable needs initializing. */

    /*
     * Information used by tkGrab.c only:
     */

    struct TkWindow *grabWinPtr;
                                /* Window in which the pointer is currently
                                 * grabbed, or NULL if none. */
    struct TkWindow *eventualGrabWinPtr;
                                /* Value that grabWinPtr will have once the
                                 * grab event queue (below) has been
                                 * completely emptied. */
    struct TkWindow *buttonWinPtr;
                                /* Window in which first mouse button was
                                 * pressed while grab was in effect, or NULL
                                 * if no such press in effect. */
    struct TkWindow *serverWinPtr;
                                /* If no application contains the pointer then
                                 * this is NULL.  Otherwise it contains the
                                 * last window for which we've gotten an
                                 * Enter or Leave event from the server (i.e.
                                 * the last window known to have contained
                                 * the pointer).  Doesn't reflect events
                                 * that were synthesized in tkGrab.c. */
    TkGrabEvent *firstGrabEventPtr;
                                /* First in list of enter/leave events
                                 * synthesized by grab code.  These events
                                 * must be processed in order before any other
                                 * events are processed.  NULL means no such
                                 * events. */
    TkGrabEvent *lastGrabEventPtr;
                                /* Last in list of synthesized events, or NULL
                                 * if list is empty. */
    int grabFlags;              /* Miscellaneous flag values.  See definitions
                                 * in tkGrab.c. */

    /*
     * Information used by tkGrid.c only:
     */

    int gridInit;               /* 0 means table below needs initializing. */
    Tcl_HashTable gridHashTable;/* Maps from Tk_Window tokens to 
                                 * corresponding Grid structures. */

    /*
     * Information used by tkImage.c only:
     */

    int imageId;                /* Value used to number image ids. */

    /*
     * Information used by tkMacWinMenu.c only:
     */

    int postCommandGeneration;  

    /*
     * Information used by tkOption.c only.
     */



    /*
     * Information used by tkPack.c only.
     */

    int packInit;              /* 0 means table below needs initializing. */
    Tcl_HashTable packerHashTable;
                               /* Maps from Tk_Window tokens to 
                                * corresponding Packer structures. */
    

    /*
     * Information used by tkPlace.c only.
     */

    int placeInit;              /* 0 means tables below need initializing. */
    Tcl_HashTable masterTable;  /* Maps from Tk_Window toke to the Master
                                 * structure for the window, if it exists. */
    Tcl_HashTable slaveTable;   /* Maps from Tk_Window toke to the Slave
                                 * structure for the window, if it exists. */

    /*
     * Information used by tkSelect.c and tkClipboard.c only:
     */

    struct TkSelectionInfo *selectionInfoPtr;
                                /* First in list of selection information
                                 * records.  Each entry contains information
                                 * about the current owner of a particular
                                 * selection on this display. */
    Atom multipleAtom;          /* Atom for MULTIPLE.  None means
                                 * selection stuff isn't initialized. */
    Atom incrAtom;              /* Atom for INCR. */
    Atom targetsAtom;           /* Atom for TARGETS. */
    Atom timestampAtom;         /* Atom for TIMESTAMP. */
    Atom textAtom;              /* Atom for TEXT. */
    Atom compoundTextAtom;      /* Atom for COMPOUND_TEXT. */
    Atom applicationAtom;       /* Atom for TK_APPLICATION. */
    Atom windowAtom;            /* Atom for TK_WINDOW. */
    Atom clipboardAtom;         /* Atom for CLIPBOARD. */

    Tk_Window clipWindow;       /* Window used for clipboard ownership and to
                                 * retrieve selections between processes. NULL
                                 * means clipboard info hasn't been
                                 * initialized. */
    int clipboardActive;        /* 1 means we currently own the clipboard
                                 * selection, 0 means we don't. */
    struct TkMainInfo *clipboardAppPtr;
                                /* Last application that owned clipboard. */
    struct TkClipboardTarget *clipTargetPtr;
                                /* First in list of clipboard type information
                                 * records.  Each entry contains information
                                 * about the buffers for a given selection
                                 * target. */

    /*
     * Information used by tkSend.c only:
     */

    Tk_Window commTkwin;        /* Window used for communication
                                 * between interpreters during "send"
                                 * commands.  NULL means send info hasn't
                                 * been initialized yet. */
    Atom commProperty;          /* X's name for comm property. */
    Atom registryProperty;      /* X's name for property containing
                                 * registry of interpreter names. */
    Atom appNameProperty;       /* X's name for property used to hold the
                                 * application name on each comm window. */

    /*
     * Information used by tkXId.c only:
     */

    struct TkIdStack *idStackPtr;
                                /* First in list of chunks of free resource
                                 * identifiers, or NULL if there are no free
                                 * resources. */
    XID (*defaultAllocProc) _ANSI_ARGS_((Display *display));
                                /* Default resource allocator for display. */
    struct TkIdStack *windowStackPtr;
                                /* First in list of chunks of window
                                 * identifers that can't be reused right
                                 * now. */
    int idCleanupScheduled;     /* 1 means a call to WindowIdCleanup has
                                 * already been scheduled, 0 means it
                                 * hasn't. */

    /*
     * Information used by tkUnixWm.c and tkWinWm.c only:
     */

    int wmTracing;              /* Used to enable or disable tracing in 
                                 * this module.  If tracing is enabled, 
                                 * then information is printed on
                                 * standard output about interesting 
                                 * interactions with the window manager. */
    struct TkWmInfo *firstWmPtr;  /* Points to first top-level window. */
    struct TkWmInfo *foregroundWmPtr;    
                                /* Points to the foreground window. */

    /*
     * Information maintained by tkWindow.c for use later on by tkXId.c:
     */


    int destroyCount;           /* Number of Tk_DestroyWindow operations
                                 * in progress. */
    unsigned long lastDestroyRequest;
                                /* Id of most recent XDestroyWindow request;
                                 * can re-use ids in windowStackPtr when
                                 * server has seen this request and event
                                 * queue is empty. */

    /*
     * Information used by tkVisual.c only:
     */

    TkColormap *cmapPtr;        /* First in list of all non-default colormaps
                                 * allocated for this display. */

    /*
     * Miscellaneous information:
     */

#ifdef TK_USE_INPUT_METHODS
    XIM inputMethod;            /* Input method for this display */
#endif /* TK_USE_INPUT_METHODS */
    Tcl_HashTable winTable;     /* Maps from X window ids to TkWindow ptrs. */

    int refCount;               /* Reference count of how many Tk applications
                                 * are using this display. Used to clean up
                                 * the display when we no longer have any
                                 * Tk applications using it.
                                 */
    /*
     * The following field were all added for Tk8.3
     */
    int mouseButtonState;       /* current mouse button state for this
                                 * display */
    int warpInProgress;
    Window warpWindow;
    int warpX;
    int warpY;
    int useInputMethods;        /* Whether to use input methods */
} TkDisplay;

/*
 * One of the following structures exists for each error handler
 * created by a call to Tk_CreateErrorHandler.  The structure
 * is managed by tkError.c.
 */

typedef struct TkErrorHandler {
    TkDisplay *dispPtr;         /* Display to which handler applies. */
    unsigned long firstRequest; /* Only errors with serial numbers
                                 * >= to this are considered. */
    unsigned long lastRequest;  /* Only errors with serial numbers
                                 * <= to this are considered.  This
                                 * field is filled in when XUnhandle
                                 * is called.  -1 means XUnhandle
                                 * hasn't been called yet. */
    int error;                  /* Consider only errors with this
                                 * error_code (-1 means consider
                                 * all errors). */
    int request;                /* Consider only errors with this
                                 * major request code (-1 means
                                 * consider all major codes). */
    int minorCode;              /* Consider only errors with this
                                 * minor request code (-1 means
                                 * consider all minor codes). */
    Tk_ErrorProc *errorProc;    /* Procedure to invoke when a matching
                                 * error occurs.  NULL means just ignore
                                 * errors. */
    ClientData clientData;      /* Arbitrary value to pass to
                                 * errorProc. */
    struct TkErrorHandler *nextPtr;
                                /* Pointer to next older handler for
                                 * this display, or NULL for end of
                                 * list. */
} TkErrorHandler;




/*
 * One of the following structures exists for each event handler
 * created by calling Tk_CreateEventHandler.  This information
 * is used by tkEvent.c only.
 */

typedef struct TkEventHandler {
    unsigned long mask;         /* Events for which to invoke
                                 * proc. */
    Tk_EventProc *proc;         /* Procedure to invoke when an event
                                 * in mask occurs. */
    ClientData clientData;      /* Argument to pass to proc. */
    struct TkEventHandler *nextPtr;
                                /* Next in list of handlers
                                 * associated with window (NULL means
                                 * end of list). */
} TkEventHandler;

/*
 * Tk keeps one of the following data structures for each main
 * window (created by a call to TkCreateMainWindow).  It stores
 * information that is shared by all of the windows associated
 * with a particular main window.
 */

typedef struct TkMainInfo {
    int refCount;               /* Number of windows whose "mainPtr" fields
                                 * point here.  When this becomes zero, can
                                 * free up the structure (the reference
                                 * count is zero because windows can get
                                 * deleted in almost any order;  the main
                                 * window isn't necessarily the last one
                                 * deleted). */
    struct TkWindow *winPtr;    /* Pointer to main window. */
    Tcl_Interp *interp;         /* Interpreter associated with application. */
    Tcl_HashTable nameTable;    /* Hash table mapping path names to TkWindow
                                 * structs for all windows related to this
                                 * main window.  Managed by tkWindow.c. */
    Tk_BindingTable bindingTable;
                                /* Used in conjunction with "bind" command
                                 * to bind events to Tcl commands. */
    TkBindInfo bindInfo;        /* Information used by tkBind.c on a per
                                 * application basis. */
    struct TkFontInfo *fontInfoPtr;
                                /* Information used by tkFont.c on a per
                                 * application basis. */

    /*
     * Information used only by tkFocus.c and tk*Embed.c:
     */

    struct TkToplevelFocusInfo *tlFocusPtr;
                                /* First in list of records containing focus
                                 * information for each top-level in the
                                 * application.  Used only by tkFocus.c. */
    struct TkDisplayFocusInfo *displayFocusPtr;
                                /* First in list of records containing focus
                                 * information for each display that this
                                 * application has ever used.  Used only
                                 * by tkFocus.c. */

    struct ElArray *optionRootPtr;
                                /* Top level of option hierarchy for this
                                 * main window.  NULL means uninitialized.
                                 * Managed by tkOption.c. */
    Tcl_HashTable imageTable;   /* Maps from image names to Tk_ImageMaster
                                 * structures.  Managed by tkImage.c. */
    int strictMotif;            /* This is linked to the tk_strictMotif
                                 * global variable. */
    struct TkMainInfo *nextPtr; /* Next in list of all main windows managed by
                                 * this process. */
} TkMainInfo;

/*
 * Tk keeps the following data structure for each of it's builtin
 * bitmaps.  This structure is only used by tkBitmap.c and other
 * platform specific bitmap files.
 */

typedef struct {
    char *source;               /* Bits for bitmap. */
    int width, height;          /* Dimensions of bitmap. */
    int native;                 /* 0 means generic (X style) bitmap,
                                 * 1 means native style bitmap. */
} TkPredefBitmap;

/*
 * Tk keeps one of the following structures for each window.
 * Some of the information (like size and location) is a shadow
 * of information managed by the X server, and some is special
 * information used here, such as event and geometry management
 * information.  This information is (mostly) managed by tkWindow.c.
 * WARNING: the declaration below must be kept consistent with the
 * Tk_FakeWin structure in tk.h.  If you change one, be sure to
 * change the other!!
 */

typedef struct TkWindow {

    /*
     * Structural information:
     */

    Display *display;           /* Display containing window. */
    TkDisplay *dispPtr;         /* Tk's information about display
                                 * for window. */
    int screenNum;              /* Index of screen for window, among all
                                 * those for dispPtr. */
    Visual *visual;             /* Visual to use for window.  If not default,
                                 * MUST be set before X window is created. */
    int depth;                  /* Number of bits/pixel. */
    Window window;              /* X's id for window.   NULL means window
                                 * hasn't actually been created yet, or it's
                                 * been deleted. */
    struct TkWindow *childList; /* First in list of child windows,
                                 * or NULL if no children.  List is in
                                 * stacking order, lowest window first.*/
    struct TkWindow *lastChildPtr;
                                /* Last in list of child windows (highest
                                 * in stacking order), or NULL if no
                                 * children. */
    struct TkWindow *parentPtr; /* Pointer to parent window (logical
                                 * parent, not necessarily X parent).  NULL
                                 * means either this is the main window, or
                                 * the window's parent has already been
                                 * deleted. */
    struct TkWindow *nextPtr;   /* Next higher sibling (in stacking order)
                                 * in list of children with same parent.  NULL
                                 * means end of list. */
    TkMainInfo *mainPtr;        /* Information shared by all windows
                                 * associated with a particular main
                                 * window.  NULL means this window is
                                 * a rogue that isn't associated with
                                 * any application (at present, this
                                 * only happens for the dummy windows
                                 * used for "send" communication).  */

    /*
     * Name and type information for the window:
     */

    char *pathName;             /* Path name of window (concatenation
                                 * of all names between this window and
                                 * its top-level ancestor).  This is a
                                 * pointer into an entry in
                                 * mainPtr->nameTable.  NULL means that
                                 * the window hasn't been completely
                                 * created yet. */
    Tk_Uid nameUid;             /* Name of the window within its parent
                                 * (unique within the parent). */
    Tk_Uid classUid;            /* Class of the window.  NULL means window
                                 * hasn't been given a class yet. */

    /*
     * Geometry and other attributes of window.  This information
     * may not be updated on the server immediately;  stuff that
     * hasn't been reflected in the server yet is called "dirty".
     * At present, information can be dirty only if the window
     * hasn't yet been created.
     */

    XWindowChanges changes;     /* Geometry and other info about
                                 * window. */
    unsigned int dirtyChanges;  /* Bits indicate fields of "changes"
                                 * that are dirty. */
    XSetWindowAttributes atts;  /* Current attributes of window. */
    unsigned long dirtyAtts;    /* Bits indicate fields of "atts"
                                 * that are dirty. */

    unsigned int flags;         /* Various flag values:  these are all
                                 * defined in tk.h (confusing, but they're
                                 * needed there for some query macros). */

    /*
     * Information kept by the event manager (tkEvent.c):
     */

    TkEventHandler *handlerList;/* First in list of event handlers
                                 * declared for this window, or
                                 * NULL if none. */
#ifdef TK_USE_INPUT_METHODS
    XIC inputContext;           /* Input context (for input methods). */
#endif /* TK_USE_INPUT_METHODS */

    /*
     * Information used for event bindings (see "bind" and "bindtags"
     * commands in tkCmds.c):
     */

    ClientData *tagPtr;         /* Points to array of tags used for bindings
                                 * on this window.  Each tag is a Tk_Uid.
                                 * Malloc'ed.  NULL means no tags. */
    int numTags;                /* Number of tags at *tagPtr. */

    /*
     * Information used by tkOption.c to manage options for the
     * window.
     */

    int optionLevel;            /* -1 means no option information is
                                 * currently cached for this window.
                                 * Otherwise this gives the level in
                                 * the option stack at which info is
                                 * cached. */
    /*
     * Information used by tkSelect.c to manage the selection.
     */

    struct TkSelHandler *selHandlerList;
                                /* First in list of handlers for
                                 * returning the selection in various
                                 * forms. */

    /*
     * Information used by tkGeometry.c for geometry management.
     */

    Tk_GeomMgr *geomMgrPtr;     /* Information about geometry manager for
                                 * this window. */
    ClientData geomData;        /* Argument for geometry manager procedures. */
    int reqWidth, reqHeight;    /* Arguments from last call to
                                 * Tk_GeometryRequest, or 0's if
                                 * Tk_GeometryRequest hasn't been
                                 * called. */
    int internalBorderWidth;    /* Width of internal border of window
                                 * (0 means no internal border).  Geometry
                                 * managers should not normally place children
                                 * on top of the border. */

    /*
     * Information maintained by tkWm.c for window manager communication.
     */

    struct TkWmInfo *wmInfoPtr; /* For top-level windows (and also
                                 * for special Unix menubar and wrapper
                                 * windows), points to structure with
                                 * wm-related info (see tkWm.c).  For
                                 * other windows, this is NULL. */

    /*
     * Information used by widget classes.
     */

    TkClassProcs *classProcsPtr;
    ClientData instanceData;

    /*
     * Platform specific information private to each port.
     */

    struct TkWindowPrivate *privatePtr;
} TkWindow;

/*
 * The following structure is used as a two way map between integers
 * and strings, usually to map between an internal C representation
 * and the strings used in Tcl.
 */

typedef struct TkStateMap {
    int numKey;                 /* Integer representation of a value. */
    char *strKey;               /* String representation of a value. */
} TkStateMap;

/*
 * This structure is used by the Mac and Window porting layers as
 * the internal representation of a clip_mask in a GC.
 */

typedef struct TkpClipMask {
    int type;                   /* One of TKP_CLIP_PIXMAP or TKP_CLIP_REGION */
    union {
        Pixmap pixmap;
        TkRegion region;
    } value;
} TkpClipMask;

#define TKP_CLIP_PIXMAP 0
#define TKP_CLIP_REGION 1

/*
 * Pointer to first entry in list of all displays currently known.
 */

extern TkDisplay *tkDisplayList;

/*
 * Return values from TkGrabState:
 */

#define TK_GRAB_NONE            0
#define TK_GRAB_IN_TREE         1
#define TK_GRAB_ANCESTOR        2
#define TK_GRAB_EXCLUDED        3

/*
 * The macro below is used to modify a "char" value (e.g. by casting
 * it to an unsigned character) so that it can be used safely with
 * macros such as isspace.
 */

#define UCHAR(c) ((unsigned char) (c))

/*
 * The following symbol is used in the mode field of FocusIn events
 * generated by an embedded application to request the input focus from
 * its container.
 */

#define EMBEDDED_APP_WANTS_FOCUS (NotifyNormal + 20)

/*
 * The following special modifier mask bits are defined, to indicate
 * logical modifiers such as Meta and Alt that may float among the
 * actual modifier bits.
 */

#define META_MASK       (AnyModifier<<1)
#define ALT_MASK        (AnyModifier<<2)

/*
 * Miscellaneous variables shared among Tk modules but not exported
 * to the outside world:
 */

extern Tk_SmoothMethod          tkBezierSmoothMethod;
extern Tk_ImageType             tkBitmapImageType;
extern Tk_PhotoImageFormat      tkImgFmtGIF;
extern void                     (*tkHandleEventProc) _ANSI_ARGS_((
                                    XEvent* eventPtr));
extern Tk_PhotoImageFormat      tkImgFmtPPM;
extern TkMainInfo               *tkMainWindowList;
extern Tk_ImageType             tkPhotoImageType;
extern Tcl_HashTable            tkPredefBitmapTable;
extern int                      tkSendSerial;

#include "tkIntDecls.h"

#ifdef BUILD_tk
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * Internal procedures shared among Tk modules but not exported
 * to the outside world:
 */

EXTERN int              Tk_AfterCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_BellObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_BindCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_BindtagsCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_ButtonObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_CanvasObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, Tcl_Obj *CONST objv[]));
EXTERN int              Tk_CheckbuttonObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_ClipboardCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_ChooseColorObjCmd _ANSI_ARGS_((
                            ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[]));
EXTERN int              Tk_ChooseDirectoryObjCmd _ANSI_ARGS_((
                            ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *CONST objv[]));
EXTERN int              Tk_ChooseFontObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_DestroyObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_EntryObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_EventObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_FileeventCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_FrameObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_FocusObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_FontObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_GetOpenFileObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_GetSaveFileObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_GrabCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_GridCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_ImageObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_LabelObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_ListboxObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_LowerObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_MenubuttonObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_MessageBoxObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_MessageCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_OptionObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_PackCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_PlaceCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_RadiobuttonObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_RaiseObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_ScaleObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_ScrollbarCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_SelectionCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_SendCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_SendObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_TextCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_TkObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_TkwaitCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              Tk_ToplevelObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_UpdateObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc, 
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_WinfoObjCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int objc,
                            Tcl_Obj *CONST objv[]));
EXTERN int              Tk_WmCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));

void    TkConsolePrint _ANSI_ARGS_((Tcl_Interp *interp,
                            int devId, char *buffer, long size));

EXTERN void             TkEventInit _ANSI_ARGS_((void));

EXTERN int              TkCreateMenuCmd _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN int              TkDeadAppCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));

EXTERN int              TkpTestembedCmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));
EXTERN int              TkCanvasGetCoordObj _ANSI_ARGS_((Tcl_Interp *interp,
                            Tk_Canvas canvas, Tcl_Obj *obj,
                            double *doublePtr));
EXTERN int              TkCanvasDashParseProc _ANSI_ARGS_((
                            ClientData clientData, Tcl_Interp *interp,
                            Tk_Window tkwin, CONST char *value, char *widgRec,
                            int offset));
EXTERN char *           TkCanvasDashPrintProc _ANSI_ARGS_((
                            ClientData clientData, Tk_Window tkwin,
                            char *widgRec, int offset,
                            Tcl_FreeProc **freeProcPtr));
EXTERN int              TkGetDoublePixels _ANSI_ARGS_((Tcl_Interp *interp,
                            Tk_Window tkwin, CONST char *string,
                            double *doublePtr));
EXTERN int              TkOffsetParseProc _ANSI_ARGS_((
                            ClientData clientData, Tcl_Interp *interp,
                            Tk_Window tkwin, CONST char *value, char *widgRec,
                            int offset));
EXTERN char *           TkOffsetPrintProc _ANSI_ARGS_((
                            ClientData clientData, Tk_Window tkwin,
                            char *widgRec, int offset,
                            Tcl_FreeProc **freeProcPtr));
EXTERN int              TkOrientParseProc _ANSI_ARGS_((
                            ClientData clientData, Tcl_Interp *interp,
                            Tk_Window tkwin, CONST char *value,
                            char *widgRec, int offset));
EXTERN char *           TkOrientPrintProc _ANSI_ARGS_((
                            ClientData clientData, Tk_Window tkwin,
                            char *widgRec, int offset,
                            Tcl_FreeProc **freeProcPtr));
EXTERN int              TkPixelParseProc _ANSI_ARGS_((
                            ClientData clientData, Tcl_Interp *interp,
                            Tk_Window tkwin, CONST char *value, char *widgRec,
                            int offset));
EXTERN char *           TkPixelPrintProc _ANSI_ARGS_((
                            ClientData clientData, Tk_Window tkwin,
                            char *widgRec, int offset,
                            Tcl_FreeProc **freeProcPtr));
EXTERN int              TkPostscriptImage _ANSI_ARGS_((Tcl_Interp *interp,
                            Tk_Window tkwin, Tk_PostscriptInfo psInfo,
                            XImage *ximage, int x, int y, int width,
                            int height));
EXTERN int              TkSmoothParseProc _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, Tk_Window tkwin,
                            CONST char *value, char *recordPtr, int offset));
EXTERN char *           TkSmoothPrintProc _ANSI_ARGS_((ClientData clientData,
                            Tk_Window tkwin, char *recordPtr, int offset,
                            Tcl_FreeProc **freeProcPtr));
EXTERN int              TkStateParseProc _ANSI_ARGS_((
                            ClientData clientData, Tcl_Interp *interp,
                            Tk_Window tkwin, CONST char *value,
                            char *widgRec, int offset));
EXTERN char *           TkStatePrintProc _ANSI_ARGS_((
                            ClientData clientData, Tk_Window tkwin,
                            char *widgRec, int offset,
                            Tcl_FreeProc **freeProcPtr));
EXTERN int              TkTileParseProc _ANSI_ARGS_((
                            ClientData clientData, Tcl_Interp *interp,
                            Tk_Window tkwin, CONST char *value, char *widgRec,
                            int offset));
EXTERN char *           TkTilePrintProc _ANSI_ARGS_((
                            ClientData clientData, Tk_Window tkwin,
                            char *widgRec, int offset,
                            Tcl_FreeProc **freeProcPtr));

/* 
 * Unsupported commands.
 */
EXTERN int              TkUnsupported1Cmd _ANSI_ARGS_((ClientData clientData,
                            Tcl_Interp *interp, int argc, char **argv));

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLIMPORT

#endif  /* _TKINT */
