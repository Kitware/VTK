#ifndef VTK_RENDER_WIDGET_DEFINED
#define VTK_RENDER_WIDGET_DEFINED


#include "vtkTclUtil.h"
#include "vtkXRenderWindow.h"

#define ALL_EVENTS_MASK \
    KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|	\
    EnterWindowMask|LeaveWindowMask|PointerMotionMask|ExposureMask|	\
    VisibilityChangeMask|FocusChangeMask|PropertyChangeMask|ColormapChangeMask
#define MAX(a,b)	(((a)>(b))?(a):(b))
    
struct Vtkrenderwidget
{
  Tk_Window  TkWin;		/* Tk window structure */
  Tcl_Interp *Interp;		/* Tcl interpreter */
  int Width;
  int Height;
  vtkRenderWindow *RenderWindow;
  char *RW;
};



//----------------------------------------------------------------------------
// Internal Tk stuff that we need to force Tk to use our window.
// It works with 4.1, but may break with the next Tk version.
//----------------------------------------------------------------------------

// We need this structure for winTable. (It would be last)
typedef struct TkDisplay {
    Display *display;		/* Xlib's info about display. */
    struct TkDisplay *nextPtr;	/* Next in list of all displays. */
    char *name;			/* Name of display (with any screen
				 * identifier removed).  Malloc-ed. */
    Time lastEventTime;		/* Time of last event received for this
				 * display. */

    /*
     * Information used primarily by tkBind.c:
     */

    int bindInfoStale;		/* Non-zero means the variables in this
				 * part of the structure are potentially
				 * incorrect and should be recomputed. */
    unsigned int modeModMask;	/* Has one bit set to indicate the modifier
				 * corresponding to "mode shift".  If no
				 * such modifier, than this is zero. */
    unsigned int metaModMask;	/* Has one bit set to indicate the modifier
				 * corresponding to the "Meta" key.  If no
				 * such modifier, then this is zero. */
    unsigned int altModMask;	/* Has one bit set to indicate the modifier
				 * corresponding to the "Meta" key.  If no
				 * such modifier, then this is zero. */
    int lockUsage;		/* BBB: enum doesn't work with C++ here... */
				/* Indicates how to interpret lock modifier. */
    int numModKeyCodes;		/* Number of entries in modKeyCodes array
				 * below. */
    void  *modKeyCodes;	        /* lawcc: KeyCode -> void
				 * Pointer to an array giving keycodes for
				 * all of the keys that have modifiers
				 * associated with them.  Malloc'ed, but
				 * may be NULL. */

    /*
     * Information used by tkError.c only:
     */

    void *errorPtr;             /* lawcc: struct TkErrorHandler -> void
				 * First in list of error handlers
				 * for this display.  NULL means
				 * no handlers exist at present. */
    int deleteCount;		/* Counts # of handlers deleted since
				 * last time inactive handlers were
				 * garbage-collected.  When this number
				 * gets big, handlers get cleaned up. */

    /*
     * Information used by tkSend.c only:
     */

    Tk_Window commTkwin;	/* Window used for communication
				 * between interpreters during "send"
				 * commands.  NULL means send info hasn't
				 * been initialized yet. */
    Atom commProperty;		/* X's name for comm property. */
    Atom registryProperty;	/* X's name for property containing
				 * registry of interpreter names. */
    Atom appNameProperty;	/* X's name for property used to hold the
				 * application name on each comm window. */

    /*
     * Information used by tkSelect.c and tkClipboard.c only:
     */

    void *selectionInfoPtr;     /* lawcc: struct TkSelectionInfo -> void
				 * First in list of selection information
				 * records.  Each entry contains information
				 * about the current owner of a particular
				 * selection on this display. */
    Atom multipleAtom;		/* Atom for MULTIPLE.  None means
				 * selection stuff isn't initialized. */
    Atom incrAtom;		/* Atom for INCR. */
    Atom targetsAtom;		/* Atom for TARGETS. */
    Atom timestampAtom;		/* Atom for TIMESTAMP. */
    Atom textAtom;		/* Atom for TEXT. */
    Atom compoundTextAtom;	/* Atom for COMPOUND_TEXT. */
    Atom applicationAtom;	/* Atom for TK_APPLICATION. */
    Atom windowAtom;		/* Atom for TK_WINDOW. */
    Atom clipboardAtom;		/* Atom for CLIPBOARD. */

    Tk_Window clipWindow;	/* Window used for clipboard ownership and to
				 * retrieve selections between processes. NULL
				 * means clipboard info hasn't been
				 * initialized. */
    int clipboardActive;	/* 1 means we currently own the clipboard
				 * selection, 0 means we don't. */
    void *clipboardAppPtr;      /* lawcc: struct TkMainInfo -> void
				 * Last application that owned clipboard. */
    void *clipTargetPtr;        /* lawcc: struct TkClipboardTarget -> void
				 * First in list of clipboard type information
				 * records.  Each entry contains information
				 * about the buffers for a given selection
				 * target. */

    /*
     * Information used by tkAtom.c only:
     */

    int atomInit;		/* 0 means stuff below hasn't been
				 * initialized yet. */
    Tcl_HashTable nameTable;	/* Maps from names to Atom's. */
    Tcl_HashTable atomTable;	/* Maps from Atom's back to names. */

    /*
     * Information used by tkCursor.c only:
     */

    Font cursorFont;		/* Font to use for standard cursors.
				 * None means font not loaded yet. */

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
    void *firstGrabEventPtr;    /* lawcc: TkGrabEvent -> void
				 * First in list of enter/leave events
				 * synthesized by grab code.  These events
				 * must be processed in order before any other
				 * events are processed.  NULL means no such
				 * events. */
    void *lastGrabEventPtr;     /* lawcc: TkGrabEvent -> void
				 * Last in list of synthesized events, or NULL
				 * if list is empty. */
    int grabFlags;		/* Miscellaneous flag values.  See definitions
				 * in tkGrab.c. */

    /*
     * Information used by tkXId.c only:
     */
	
    void *idStackPtr;           /* lawcc: struct TkIdStack -> void
				 * First in list of chunks of free resource
				 * identifiers, or NULL if there are no free
				 * resources. */
    XID (*defaultAllocProc) _ANSI_ARGS_((Display *display));
				/* Default resource allocator for display. */
    void *windowStackPtr;       /* lawcc: struct TkIdStack -> void
				 * First in list of chunks of window
				 * identifers that can't be reused right
				 * now. */
    int idCleanupScheduled;	/* 1 means a call to WindowIdCleanup has
				 * already been scheduled, 0 means it
				 * hasn't. */

    /*
     * Information maintained by tkWindow.c for use later on by tkXId.c:
     */


    int destroyCount;		/* Number of Tk_DestroyWindow operations
				 * in progress. */
    unsigned long lastDestroyRequest;
				/* Id of most recent XDestroyWindow request;
				 * can re-use ids in windowStackPtr when
				 * server has seen this request and event
				 * queue is empty. */

    /*
     * Information used by tkVisual.c only:
     */

    void  *cmapPtr;             /* lawcc: TkColormap *cmapPtr;	
				 * First in list of all non-default colormaps
				 * allocated for this display. */

    /*
     * Information used by tkFocus.c only:
     */

    struct TkWindow *focusWinPtr;
				/* Window that currently has the focus for
				 * this display, or NULL if none. */
    struct TkWindow *implicitWinPtr;
				/* If the focus arrived at a toplevel window
				 * implicitly via an Enter event (rather
				 * than via a FocusIn event), this points
				 * to the toplevel window.  Otherwise it is
				 * NULL. */
    struct TkWindow *focusOnMapPtr;
				/* This points to a toplevel window that is
				 * supposed to receive the X input focus as
				 * soon as it is mapped (needed to handle the
				 * fact that X won't allow the focus on an
				 * unmapped window).  NULL means no delayed
				 * focus op in progress. */
    int forceFocus;		/* Associated with focusOnMapPtr:  non-zero
				 * means claim the focus even if some other
				 * application currently has it. */

    /*
     * Used by tkColor.c only:
     */

    void *stressPtr;            /* lawcc: TkStressedCmap *stressPtr;	
				 * First in list of colormaps that have
				 * filled up, so we have to pick an
				 * approximate color. */

    /*
     * Used by tkEvent.c only:
     */

    void *delayedMotionPtr;     /* lawcc: struct TkWindowEvent -> void
				 * Points to a malloc-ed motion event
				 * whose processing has been delayed in
				 * the hopes that another motion event
				 * will come along right away and we can
				 * merge the two of them together.  NULL
				 * means that there is no delayed motion
				 * event. */

    /*
     * Miscellaneous information:
     */

#ifdef TK_USE_INPUT_METHODS
    XIM inputMethod;		/* Input method for this display */
#endif /* TK_USE_INPUT_METHODS */
    Tcl_HashTable winTable;	/* Maps from X window ids to TkWindow ptrs. */
} TkDisplay;





typedef struct TkWindow {

    /*
     * Structural information:
     */

    Display *display;		/* Display containing window. */
    TkDisplay *dispPtr;		/* Tk's information about display
				 * for window. */
    int screenNum;		/* Index of screen for window, among all
				 * those for dispPtr. */
    Visual *visual;		/* Visual to use for window.  If not default,
				 * MUST be set before X window is created. */
    int depth;			/* Number of bits/pixel. */
    Window window;		/* X's id for window.   NULL means window
				 * hasn't actually been created yet, or it's
				 * been deleted. */
    struct TkWindow *childList;	/* First in list of child windows,
				 * or NULL if no children. */
    struct TkWindow *lastChildPtr;
				/* Last in list of child windows, or NULL
				 * if no children. */
    struct TkWindow *parentPtr;	/* Pointer to parent window (logical
				 * parent, not necessarily X parent).  NULL
				 * means either this is the main window, or
				 * the window's parent has already been
				 * deleted. */
    struct TkWindow *nextPtr;	/* Next in list of children with
				 * same parent (NULL if end of
				 * list). */
    void *mainPtr;	        /* lawcc: TkMainInfo *mainPtr;	
				 * Information shared by all windows
				 * associated with a particular main
				 * window.  NULL means this window is
				 * a rogue that isn't associated with
				 * any application (at present, this
				 * only happens for the dummy windows
				 * used for "send" communication).  */

    /*
     * Name and type information for the window:
     */

    char *pathName;		/* Path name of window (concatenation
				 * of all names between this window and
				 * its top-level ancestor).  This is a
				 * pointer into an entry in
				 * mainPtr->nameTable.  NULL means that
				 * the window hasn't been completely
				 * created yet. */
    Tk_Uid nameUid;		/* Name of the window within its parent
				 * (unique within the parent). */
    Tk_Uid classUid;		/* Class of the window.  NULL means window
				 * hasn't been given a class yet. */

    /*
     * Geometry and other attributes of window.  This information
     * may not be updated on the server immediately;  stuff that
     * hasn't been reflected in the server yet is called "dirty".
     * At present, information can be dirty only if the window
     * hasn't yet been created.
     */

    XWindowChanges changes;	/* Geometry and other info about
				 * window. */
    unsigned int dirtyChanges;	/* Bits indicate fields of "changes"
				 * that are dirty. */
    XSetWindowAttributes atts;	/* Current attributes of window. */
    unsigned long dirtyAtts;	/* Bits indicate fields of "atts"
				 * that are dirty. */

    unsigned int flags;		/* Various flag values:  these are all
				 * defined in tk.h (confusing, but they're
				 * needed there for some query macros). */

    /*
     * Information kept by the event manager (tkEvent.c):
     */

    void *handlerList;          /* First in list of event handlers
				 * declared for this window, or
				 * NULL if none. */
#ifdef TK_USE_INPUT_METHODS
    XIC inputContext;		/* Input context (for input methods). */
#endif /* TK_USE_INPUT_METHODS */
} TkWindow;

extern "C" {void TkWmAddToColormapWindows(TkWindow *winPtr);}




#endif
