/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTkWidgetsInit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTcl.h"
#include "vtkTk.h"

#include "vtkTkImageViewerWidget.h"
#include "vtkTkRenderWidget.h"
#include "vtkImageData.h"


//-----  This hack needed to compile using gcc3 on OSX until new stdc++.dylib
#ifdef __APPLE_CC__
extern "C"
{
  void oft_initRenderingWidgets()
  {
    extern void _ZNSt8ios_base4InitC4Ev();
    _ZNSt8ios_base4InitC4Ev();
  }
}
#endif


//----------------------------------------------------------------------------
// Vtkrenderingpythontkwidgets_Init
// Called upon system startup to create the widget commands.
extern "C" {VTK_TK_EXPORT int Vtkrenderingpythontkwidgets_Init(Tcl_Interp *interp);}

extern "C" 
{
  int vtkTkRenderWidget_Cmd(ClientData clientData, Tcl_Interp *interp, 
                            int argc, 
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4 && TCL_RELEASE_LEVEL >= TCL_FINAL_RELEASE)
                            CONST84
#endif
                            char **argv);

  int vtkTkImageViewerWidget_Cmd(ClientData clientData, Tcl_Interp *interp, 
                                 int argc, 
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4 && TCL_RELEASE_LEVEL >= TCL_FINAL_RELEASE)
                                 CONST84
#endif
                                 char **argv);
  int vtkImageDataToTkPhoto_Cmd (ClientData clientData,
                                         Tcl_Interp *interp, 
                                         int argc, 
#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4 && TCL_RELEASE_LEVEL >= TCL_FINAL_RELEASE)
                                 CONST84
#endif
                                 char **argv);
}

int Vtkrenderingpythontkwidgets_Init(Tcl_Interp *interp)
{
  if (Tcl_PkgProvide(interp, (char *) "Vtkrenderingpythontkwidgets", (char *) "1.2") != TCL_OK) 
    {
    return TCL_ERROR;
    }
  
  Tcl_CreateCommand(interp, (char *) "vtkTkRenderWidget", vtkTkRenderWidget_Cmd, 
                    Tk_MainWindow(interp), NULL);
  Tcl_CreateCommand(interp, (char *) "vtkTkImageViewerWidget", 
                    vtkTkImageViewerWidget_Cmd, Tk_MainWindow(interp), NULL);
  
  Tcl_CreateCommand(interp, (char *) "vtkImageDataToTkPhoto", vtkImageDataToTkPhoto_Cmd, 
                    NULL, NULL );
  return TCL_OK;
}

