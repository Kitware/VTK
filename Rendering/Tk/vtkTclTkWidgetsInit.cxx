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
#include "vtkVersionMacros.h"
#include "vtkTkImageViewerWidget.h"
#include "vtkTkRenderWidget.h"
#include "vtkImageData.h"


//----------------------------------------------------------------------------
// Vtkrenderingtcltkwidgets_Init
// Called upon system startup to create the widget commands.
extern "C" {VTK_TK_EXPORT int Vtkrenderingtktcl_Init(Tcl_Interp *interp);}

extern "C" {VTK_TK_EXPORT int Vtktkrenderwidget_Init(Tcl_Interp *interp);}
extern "C" {VTK_TK_EXPORT int Vtktkimageviewerwidget_Init(Tcl_Interp *interp);}

#define VTKTK_TO_STRING(x) VTKTK_TO_STRING0(x)
#define VTKTK_TO_STRING0(x) VTKTK_TO_STRING1(x)
#define VTKTK_TO_STRING1(x) #x
#define VTKTK_VERSION VTKTK_TO_STRING(VTK_MAJOR_VERSION) "." VTKTK_TO_STRING(VTK_MINOR_VERSION)

int Vtkrenderingtktcl_Init(Tcl_Interp *interp)
{
  // Forward the call to the real init functions.
  if(Vtktkrenderwidget_Init(interp) == TCL_OK &&
     Vtktkimageviewerwidget_Init(interp) == TCL_OK)
  {
    // Report that the package is provided.
    return Tcl_PkgProvide(interp, (char*)"vtkRenderingTkTCL",
        (char*)VTKTK_VERSION);
  }
  else
  {
    // One of the widgets is not provided.
    return TCL_ERROR;
  }
}
