/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTkWidgetsInit.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <tcl.h>
#include <tk.h>

#include "vtkTkImageViewerWidget.h"
#include "vtkTkRenderWidget.h"

//----------------------------------------------------------------------------
// Vtkrenderingpythontkwidgets_Init
// Called upon system startup to create the widget commands.
extern "C" {VTK_TK_EXPORT int Vtkrenderingpythontkwidgets_Init(Tcl_Interp *interp);}

extern "C" 
{
  int vtkTkRenderWidget_Cmd(ClientData clientData, Tcl_Interp *interp, 
                            int argc, char **argv);
  int vtkTkImageViewerWidget_Cmd(ClientData clientData, Tcl_Interp *interp, 
                                 int argc, char **argv);
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
  
  return TCL_OK;
}
