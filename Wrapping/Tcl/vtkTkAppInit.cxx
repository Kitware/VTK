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

#if defined(CMAKE_INTDIR)
# define VTK_TCL_PACKAGE_DIR VTK_TCL_PACKAGE_DIR_BUILD "/" CMAKE_INTDIR
#else
# define VTK_TCL_PACKAGE_DIR VTK_TCL_PACKAGE_DIR_BUILD
#endif

#ifdef VTK_USE_RENDERING
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

  Tcl_FindExecutable(argv[0]);

#ifdef VTK_USE_RENDERING
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

// if VTK_DISABLE_TK_INIT is defined, then those widgets were *not*
// initialized by the Rendering kit, thus we need to do it here so
// that we can use them from the vtk executable
// Also, Cocoa does not suport those widgets yet

#if defined(VTK_DISABLE_TK_INIT) && !defined(VTK_USE_COCOA)
extern "C" int Vtktkrenderwidget_Init(Tcl_Interp *interp);
extern "C" int Vtktkimageviewerwidget_Init(Tcl_Interp *interp);
#endif

#endif

#ifdef VTK_USE_VOLUMERENDERING
extern "C" int Vtkvolumerenderingtcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_HYBRID
extern "C" int Vtkhybridtcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_WIDGETS
extern "C" int Vtkwidgetstcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_PARALLEL
extern "C" int Vtkparalleltcl_Init(Tcl_Interp *interp);
#endif

void help()
{
}

#ifdef VTK_TCL_TK_COPY_SUPPORT_LIBRARY
int vtkTkAppInitFileExists(const char *filename)
{
  struct stat fs;
  return (filename && !stat(filename, &fs)) ? 1 : 0;
}

const char* vtkTkAppInitGetFilenamePath(const char *filename, char *path)
{
  if (!path)
    {
    return path;
    }

  if (!filename || !strlen(filename))
    {
    path[0] = '\0';
    return path;
    }

  const char *ptr = filename + strlen(filename) - 1;
  while (ptr > filename && *ptr != '/' && *ptr != '\\')
    {
    ptr--;
    }

  size_t length = ptr - filename;
  if (length)
    {
    strncpy(path, filename, length);
    }
  path[length] = '\0';

  return path;
}

const char* vtkTkAppInitConvertToUnixSlashes(const char* path, char *unix_path)
{
  if (!unix_path)
    {
    return unix_path;
    }

  unix_path[0] = '\0';
  if (!path)
    {
    return unix_path;
    }

  if (path[0] == '~')
    {
    const char* home = getenv("HOME");
    if(home)
      {
      strcpy(unix_path, home);
      }
    }
  strcat(unix_path, path);

  size_t length = strlen(unix_path);
  if (length < 1)
    {
    return unix_path;
    }

  size_t i;
  for (i = 0; i < length; ++i)
    {
    if(unix_path[i] == '\\')
      {
      unix_path[i] = '/';
      }
    }

  if (unix_path[length - 1] == '/')
    {
    unix_path[length - 1] = '\0';
    }

  return unix_path;
}
#endif

int Tcl_AppInit(Tcl_Interp *interp)
{
#ifdef __CYGWIN__
  Tcl_SetVar(interp, "tclDefaultLibrary", "/usr/share/tcl" TCL_VERSION, TCL_GLOBAL_ONLY);
#endif

  /*
    Tcl/Tk requires support files to work (set of tcl files).
    When an app is linked against Tcl/Tk shared libraries, the path to
    the libraries is used by Tcl/Tk to search for its support files.
    For example, on Windows, if bin/tcl84.dll is the shared lib, support
    files will be searched in bin/../lib/tcl8.4, which is where they are
    usually installed.
    If an app is linked against Tcl/Tk *static* libraries, there is no
    way for Tcl/Tk to find its support files. In that case, it will
    use the TCL_LIBRARY and TK_LIBRARY environment variable (those should
    point to the support files dir, ex: c:/tcl/lib/tcl8.4, c:/tk/lib/tcl8.4).

    The above code will also make Tcl/Tk search inside VTK's build/install
    directory, more precisely inside a TclTk/lib sub dir.
    ex: [path to vtk.exe]/TclTk/lib/tcl8.4, [path to vtk.exe]/TclTk/lib/tk8.4
    Support files are copied to that location when
    VTK_TCL_TK_COPY_SUPPORT_LIBRARY is ON.
  */

#ifdef VTK_TCL_TK_COPY_SUPPORT_LIBRARY
  int has_tcllibpath_env = getenv("TCL_LIBRARY") ? 1 : 0;
  int has_tklibpath_env = getenv("TK_LIBRARY") ? 1 : 0;
  if (!has_tcllibpath_env || !has_tklibpath_env)
    {
    const char *nameofexec = Tcl_GetNameOfExecutable();
    if (nameofexec && vtkTkAppInitFileExists(nameofexec))
      {
      char dir[1024], dir_unix[1024], buffer[1024];
      vtkTkAppInitGetFilenamePath(nameofexec, dir);
      vtkTkAppInitConvertToUnixSlashes(dir, dir_unix);

      // Installed application, otherwise build tree/windows
      sprintf(buffer, "%s/TclTk", dir_unix);
      int exists = vtkTkAppInitFileExists(buffer);
      if (!exists)
        {
        sprintf(buffer, "%s/.." VTK_TCL_INSTALL_LIB_DIR "/TclTk", dir_unix);
        exists = vtkTkAppInitFileExists(buffer);
        }
      if (exists)
        {
        // Also prepend our Tcl Tk lib path to the library paths
        // This *is* mandatory if we want encodings files to be found, as they
        // are searched by browsing TclGetLibraryPath().
        // (nope, updating the Tcl tcl_libPath var won't do the trick)

        Tcl_Obj *new_libpath = Tcl_NewObj();

        if (!has_tcllibpath_env)
          {
          char tcl_library[1024] = "";
          sprintf(tcl_library, "%s/lib/tcl%s", buffer, TCL_VERSION);
          if (vtkTkAppInitFileExists(tcl_library))
            {
            // Setting TCL_LIBRARY won't do the trick, it's too late
            Tcl_SetVar(interp, "tcl_library", tcl_library,
                       TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
            Tcl_Obj *obj = Tcl_NewStringObj(tcl_library, -1);
            if (obj)
              {
              Tcl_ListObjAppendElement(interp, new_libpath, obj);
              }
            }
          }

#ifdef VTK_USE_RENDERING
        if (!has_tklibpath_env)
          {
          char tk_library[1024] = "";
          sprintf(tk_library, "%s/lib/tk%s", buffer, TK_VERSION);
          if (vtkTkAppInitFileExists(tk_library))
            {
            // Setting TK_LIBRARY won't do the trick, it's too late
            Tcl_SetVar(interp, "tk_library", tk_library,
                       TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
            Tcl_Obj *obj = Tcl_NewStringObj(tk_library, -1);
            if (obj)
              {
              Tcl_ListObjAppendElement(interp, new_libpath, obj);
              }
            }
          }
#endif
        TclSetLibraryPath(new_libpath);
        }
      }
    }
#endif

  if (Tcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }

#ifdef VTK_USE_RENDERING
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

  // if VTK_DISABLE_TK_INIT is defined, then those widgets were *not*
  // initialized by the Rendering kit, thus we need to do it here so
  // that we can use them from the vtk executable
  // Also, Cocoa does not suport those widgets yet

#if defined(VTK_DISABLE_TK_INIT) && !defined(VTK_USE_COCOA)
  if (Vtktkrenderwidget_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
  if (Vtktkimageviewerwidget_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
#endif
#endif

#ifdef VTK_USE_VOLUMERENDERING
  if (Vtkvolumerenderingtcl_Init(interp) == TCL_ERROR)
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

#ifdef VTK_USE_WIDGETS
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
 
#ifdef VTK_EXTRA_TCL_INIT
  VTK_EXTRA_TCL_INIT;
#endif

  /*
   * Append path to VTK packages to auto_path
   */
  static char script[] =
    "foreach dir [list {" VTK_TCL_PACKAGE_DIR "} {" VTK_TCL_INSTALL_DIR "}"
    " [file join [file dirname [file dirname [info nameofexecutable]]] Wrapping Tcl]"
    " [file join [file dirname [file dirname [info nameofexecutable]]] lib vtk tcl]"
    " ] {\n"
    "  if {[file isdirectory $dir]} {\n"
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
    " site-specific \"" VTK_TCL_INSTALL_DIR "\" directory.}\n"
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
