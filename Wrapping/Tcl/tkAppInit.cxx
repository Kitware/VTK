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

#include "tk.h"

#include "vtkSystemIncludes.h"
#include "vtkToolkits.h"

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
int
main(int argc, char **argv)
{
  ios::sync_with_stdio();
#ifdef VTK_USE_RENDERING
  Tk_Main(argc, argv, Tcl_AppInit);
#else
  Tcl_Main(argc, argv, Tcl_AppInit);
#endif
  return 0;			/* Needed only to prevent compiler warning. */
}


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

extern "C" int Vtkcommontcl_Init(Tcl_Interp *interp);
extern "C" int Vtkfilteringtcl_Init(Tcl_Interp *interp);
extern "C" int Vtkimagingtcl_Init(Tcl_Interp *interp);
extern "C" int Vtkgraphicstcl_Init(Tcl_Interp *interp);
extern "C" int Vtkiotcl_Init(Tcl_Interp *interp);

#ifdef VTK_USE_RENDERING
extern "C" int Vtkrenderingtcl_Init(Tcl_Interp *interp);
#ifdef VTK_USE_TKWIDGET
extern "C" int Vtktkrenderwidget_Init(Tcl_Interp *interp);
extern "C" int Vtktkimageviewerwidget_Init(Tcl_Interp *interp);
extern "C" int Vtktkimagewindowwidget_Init(Tcl_Interp *interp);
#endif
#endif

#ifdef VTK_USE_PATENTED
extern "C" int Vtkpatentedtcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_HYBRID
extern "C" int Vtkhybridtcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_PARALLEL
extern "C" int Vtkparalleltcl_Init(Tcl_Interp *interp);
#endif

int Tcl_AppInit(Tcl_Interp *interp)
{
  if (Tcl_Init(interp) == TCL_ERROR) {
  return TCL_ERROR;
  }
#ifdef VTK_USE_RENDERING
  if (Tk_Init(interp) == TCL_ERROR) {
  return TCL_ERROR;
  }
#endif
  /* init the core vtk stuff */
  if (Vtkcommontcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
  if (Vtkfilteringtcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
  if (Vtkimagingtcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
  if (Vtkgraphicstcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
  if (Vtkiotcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
    
#ifdef VTK_USE_RENDERING
  if (Vtkrenderingtcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#ifdef VTK_USE_TKWIDGET
  if (Vtktkrenderwidget_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
  if (Vtktkimagewindowwidget_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
  if (Vtktkimageviewerwidget_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#endif
#endif

#ifdef VTK_USE_PATENTED
  if (Vtkpatentedtcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#endif

#ifdef VTK_USE_HYBRID
  if (Vtkhybridtcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#endif

#ifdef VTK_USE_PARALLEL
  if (Vtkparalleltcl_Init(interp) == TCL_ERROR) 
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
  
#if (((TK_MAJOR_VERSION == 4)&&(TK_MINOR_VERSION >= 1))||((TK_MAJOR_VERSION == 8)&&(TK_MINOR_VERSION >= 0)))
  Tcl_SetVar(interp,
	     (char *) "tcl_rcFileName",
	     (char *) "~/.vtkrc",
	     TCL_GLOBAL_ONLY);
#else
    tcl_RcFileName = (char *) "~/.vtkrc";
#endif
    return TCL_OK;
}






