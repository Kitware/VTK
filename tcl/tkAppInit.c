/* 
 * tkAppInit.c --
 *
 *	Provides a default version of the Tcl_AppInit procedure for
 *	use in wish and similar Tk-based applications.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef lint
static char sccsid[] = "@(#) tkAppInit.c 1.15 95/06/28 13:14:28";
#endif /* not lint */

#include "tk.h"

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tk_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *----------------------------------------------------------------------
 */
#if (TK_MAJOR_VERSION == 3)

EXTERN int main _ANSI_ARGS_((int     argc,
                             char  **argv));
int (*tclDummyMainPtr)() = (int (*)()) main;

#if defined(DOMAIN) && defined(SING)
EXTERN int matherr _ANSI_ARGS_((struct exception *));
int (*tclDummyMathPtr)() = (int (*)()) matherr;
#endif

#else
/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */
extern int matherr();
int *tclDummyMathPtr = (int *) matherr;

int
main(int argc, char **argv)
{
    Tk_Main(argc, argv, Tcl_AppInit);
    return 0;			/* Needed only to prevent compiler warning. */
}

#endif

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int Tcl_AppInit(Tcl_Interp *interp)
{
  Tk_Window main;
  
  if (Tcl_Init(interp) == TCL_ERROR) {
  return TCL_ERROR;
  }
  if (Tk_Init(interp) == TCL_ERROR) {
  return TCL_ERROR;
  }
  
  /* init the core vtk stuff */
  if (Vtkcommontcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
    
#ifdef USE_GRAPHICS
  if (Vtkgraphicstcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#endif

#ifdef USE_IMAGING
  if (Vtkimagingtcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#endif

#ifdef USE_VOLUME
  if (Vtkvolumetcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#endif

#ifdef USE_BORESCOPE
  if (Vtkborescopetcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#endif

  /*
   * Specify a user-specific startup file to invoke if the application
   * is run interactively.  Typically the startup file is "~/.apprc"
   * where "app" is the name of the application.  If this line is deleted
   * then no user-specific startup file will be run under any conditions.
   */
  
#if ((TK_MAJOR_VERSION == 4)&&(TK_MINOR_VERSION == 1))
    Tcl_SetVar(interp, "tcl_rcFileName", "~/.wishrc", TCL_GLOBAL_ONLY);
#else
    tcl_RcFileName = "~/.wishrc";
#endif
    return TCL_OK;
}






