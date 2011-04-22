/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkTkAppInit.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * tkAppInit.c --
 *
 *      Provides a default version of the Tcl_AppInit procedure for
 *      use in wish and similar Tk-based applications.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifdef VTK_COMPILED_USING_MPI
# include <mpi.h>
# include "vtkMPIController.h"
#endif // VTK_COMPILED_USING_MPI

#include "vtkConfigure.h"
#include "vtkSystemIncludes.h"
#include "vtkToolkits.h"
#include "Wrapping/Tcl/vtkTkAppInitConfigure.h"

#ifdef VTK_TCL_TK_COPY_SUPPORT_LIBRARY
#include <sys/stat.h>
#endif

#ifdef VTK_USE_TK
# include "vtkTk.h"
#else
# include "vtkTcl.h"
#endif

/*
 * Make sure all the kits register their classes with vtkInstantiator.
 */
#include "vtkCommonInstantiator.h"
#include "vtkFilteringInstantiator.h"
#include "vtkIOInstantiator.h"
#include "vtkImagingInstantiator.h"
#include "vtkGraphicsInstantiator.h"

#ifdef VTK_USE_RENDERING
#include "vtkRenderingInstantiator.h"
#include "vtkVolumeRenderingInstantiator.h"
#include "vtkHybridInstantiator.h"
#include "vtkWidgetsInstantiator.h"
#endif

#ifdef VTK_USE_PARALLEL
#include "vtkParallelInstantiator.h"
#endif

#ifdef VTK_USE_GEOVIS
#include "vtkGeovisInstantiator.h"
#endif

#ifdef VTK_USE_INFOVIS
#include "vtkInfovisInstantiator.h"
#endif

#ifdef VTK_USE_VIEWS
#include "vtkViewsInstantiator.h"
#endif

#include "vtkTclUtil.h"

static void vtkTkAppInitEnableMSVCDebugHook();

// I need those two Tcl functions. They usually are declared in tclIntDecls.h,
// but Unix build do not have access to VTK's tkInternals include path.
// Since the signature has not changed for years (at least since 8.2),
// let's just prototype them.

EXTERN Tcl_Obj* TclGetLibraryPath _ANSI_ARGS_((void));
EXTERN void TclSetLibraryPath _ANSI_ARGS_((Tcl_Obj * pathPtr));

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *      This is the main program for the application.
 *
 * Results:
 *      None: Tk_Main never returns here, so this procedure never
 *      returns either.
 *
 * Side effects:
 *      Whatever the application does.
 *
 *----------------------------------------------------------------------
 */
#ifdef VTK_COMPILED_USING_MPI
class vtkMPICleanup {
public:
  vtkMPICleanup()
    {
      this->Controller = 0;
    }
  void Initialize(int *argc, char ***argv)
    {
      MPI_Init(argc, argv);
      this->Controller = vtkMPIController::New();
      this->Controller->Initialize(argc, argv, 1);
      vtkMultiProcessController::SetGlobalController(this->Controller);
    }
  ~vtkMPICleanup()
    {
      if ( this->Controller )
        {
        this->Controller->Finalize();
        this->Controller->Delete();
        }
    }
private:
  vtkMPIController *Controller;
};

static vtkMPICleanup VTKMPICleanup;

#endif // VTK_COMPILED_USING_MPI

int
main(int argc, char **argv)
{
  ios::sync_with_stdio();
  vtkTkAppInitEnableMSVCDebugHook();

#ifdef VTK_COMPILED_USING_MPI
  VTKMPICleanup.Initialize(&argc, &argv);
#endif // VTK_COMPILED_USING_MPI

  // This is mandatory *now*, it does more than just finding the executable
  // (like finding the encodings, setting variables depending on the value
  // of TCL_LIBRARY, TK_LIBRARY

  vtkTclApplicationInitExecutable(argc, argv);

#ifdef VTK_USE_TK
  Tk_Main(argc, argv, Tcl_AppInit);
#else
  Tcl_Main(argc, argv, Tcl_AppInit);
#endif

  return 0;                  /* Needed only to prevent compiler warning. */
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *      This procedure performs application-specific initialization.
 *      Most applications, especially those that incorporate additional
 *      packages, will have their own version of this procedure.
 *
 * Results:
 *      Returns a standard Tcl completion code, and leaves an error
 *      message in interp->result if an error occurs.
 *
 * Side effects:
 *      Depends on the startup script.
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

#if defined(VTK_USE_TK)
extern "C" int Vtktkrenderwidget_Init(Tcl_Interp *interp);
extern "C" int Vtktkimageviewerwidget_Init(Tcl_Interp *interp);
#endif

extern "C" int Vtkvolumerenderingtcl_Init(Tcl_Interp *interp);
extern "C" int Vtkhybridtcl_Init(Tcl_Interp *interp);
extern "C" int Vtkwidgetstcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_PARALLEL
extern "C" int Vtkparalleltcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_GEOVIS
extern "C" int Vtkgeovistcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_INFOVIS
extern "C" int Vtkinfovistcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_VIEWS
extern "C" int Vtkviewstcl_Init(Tcl_Interp *interp);
#endif

void help()
{
}

int Tcl_AppInit(Tcl_Interp *interp)
{
#ifdef __CYGWIN__
  Tcl_SetVar(interp, "tclDefaultLibrary", "/usr/share/tcl" TCL_VERSION, TCL_GLOBAL_ONLY);
#endif

  // Help Tcl find the Tcl/Tk helper files.
  const char* relative_dirs[] =
    {
      "TclTk/lib",
      ".." VTK_INSTALL_TCL_DIR,
      0
    };
  vtkTclApplicationInitTclTk(interp, relative_dirs);

  if (Tcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }

#ifdef VTK_USE_TK
  if (Tk_Init(interp) == TCL_ERROR)
    {
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

#if defined(VTK_USE_TK)
  if (Vtktkrenderwidget_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
  if (Vtktkimageviewerwidget_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
#endif

  if (Vtkvolumerenderingtcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }

  if (Vtkhybridtcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }

  if (Vtkwidgetstcl_Init(interp) == TCL_ERROR)
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
 
#ifdef VTK_USE_GEOVIS
  if (Vtkgeovistcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
#endif

#ifdef VTK_USE_INFOVIS
  if (Vtkinfovistcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
#endif
 
#ifdef VTK_USE_VIEWS
  if (Vtkviewstcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
#endif
 
#ifdef VTK_EXTRA_TCL_INIT
  VTK_EXTRA_TCL_INIT;
#endif

  /*
   * Append path to VTK packages to auto_path
   */
  static char script[] =
    "foreach dir [list "
    " [file dirname [file dirname [info nameofexecutable]]]"
#if defined(CMAKE_INTDIR)
    " [file join [file dirname [file dirname [file dirname [info nameofexecutable]]]] Wrapping Tcl " CMAKE_INTDIR "]"
#else
    " [file join [file dirname [file dirname [info nameofexecutable]]] Wrapping Tcl]"
#endif
    " ] {\n"
    "  if {[file isdirectory \"$dir\"]} {\n"
    "    lappend auto_path $dir\n"
    "  }\n"
    "}\n"
    "rename package package.orig\n"
    "proc package {args} {\n"
    "  if {[catch {set package_res [eval package.orig $args]} catch_res]} {\n"
    "    global errorInfo env\n"
    "    if {[lindex $args 0] == \"require\"} {\n"
    "      set expecting {can\'t find package vtk}\n"
    "      if {![string compare -length [string length $expecting] $catch_res $expecting]} {\n"
    "        set msg {The Tcl interpreter was probably not able to find the"
    " VTK packages.  Please check that your TCLLIBPATH environment variable"
    " includes the path to your VTK Tcl directory.  You might find it under"
    " your VTK binary directory in Wrapping/Tcl, or under your"
    " site-specific installation directory.}\n"
    "        if {[info exists env(TCLLIBPATH)]} {\n"
    "          append msg \"  The TCLLIBPATH current value is: $env(TCLLIBPATH).\"\n"
    "        }\n"
    "        set errorInfo \"$msg  The TCLLIBPATH variable is a set of"
    " space separated paths (hence, path containing spaces should be"
    " surrounded by quotes first). Windows users should use forward"
    " (Unix-style) slashes \'/\' instead of the usual backward slashes. "
    " More informations can be found in the Wrapping/Tcl/README source"
    " file (also available online at"
    " http://www.vtk.org/cgi-bin/cvsweb.cgi/~checkout~/VTK/Wrapping/Tcl/README).\n"
    "$errorInfo\"\n"
    "      }\n"
    "    }\n"
    "  error $catch_res $errorInfo\n"
    "  }\n"
    "  return $package_res\n"
    "}\n";
  Tcl_Eval(interp, script);

  /*
   * Specify a user-specific startup file to invoke if the application
   * is run interactively.  Typically the startup file is "~/.apprc"
   * where "app" is the name of the application.  If this line is deleted
   * then no user-specific startup file will be run under any conditions.
   */

  Tcl_SetVar(interp,
             (char *) "tcl_rcFileName",
             (char *) "~/.vtkrc",
             TCL_GLOBAL_ONLY);
  return TCL_OK;
}

// For a DEBUG build on MSVC, add a hook to prevent error dialogs when
// being run from DART.
#if defined(_MSC_VER) && defined(_DEBUG)
# include <crtdbg.h>
static int vtkTkAppInitDebugReport(int, char* message, int*)
{
  fprintf(stderr, message);
  exit(1);
}
void vtkTkAppInitEnableMSVCDebugHook()
{
  if(getenv("DART_TEST_FROM_DART"))
    {
    _CrtSetReportHook(vtkTkAppInitDebugReport);
    }
}
#else
void vtkTkAppInitEnableMSVCDebugHook()
{
}
#endif
