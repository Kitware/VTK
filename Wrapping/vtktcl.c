/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtktcl.c
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

#include "tk.h"
#include "vtkToolkits.h"

extern Vtkcommontcl_Init(Tcl_Interp *interp);

#ifdef VTK_USE_GRAPHICS
extern Vtkgraphicstcl_Init(Tcl_Interp *interp);
#ifdef VTK_USE_TKWIDGET
extern Vtktkrenderwidget_Init(Tcl_Interp *interp);
#endif
#endif

#ifdef VTK_USE_IMAGING
extern Vtkimagingtcl_Init(Tcl_Interp *interp);
#ifdef VTK_USE_TKWIDGET
extern Vtktkimagewindowwidget_Init(Tcl_Interp *interp);
extern Vtktkimageviewerwidget_Init(Tcl_Interp *interp);
#endif
#endif

#ifdef VTK_USE_PATENTED
extern Vtkpatentedtcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_CONTRIB
extern Vtkcontribtcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_LOCAL
extern Vtklocaltcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_PARALLEL
extern Vtkparalleltcl_Init(Tcl_Interp *interp);
#endif

int Vtktcl_Init(Tcl_Interp *interp)
{
  /* init the core vtk stuff */
  if (Vtkcommontcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
    
#ifdef VTK_USE_GRAPHICS
  if (Vtkgraphicstcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#ifdef VTK_USE_TKWIDGET
  if (Vtktkrenderwidget_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#endif
#endif

#ifdef VTK_USE_IMAGING
  if (Vtkimagingtcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#ifdef VTK_USE_TKWIDGET
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

#ifdef VTK_USE_CONTRIB
  if (Vtkcontribtcl_Init(interp) == TCL_ERROR) 
    {
    return TCL_ERROR;
    }
#endif

#ifdef VTK_USE_LOCAL
  if (Vtklocaltcl_Init(interp) == TCL_ERROR) 
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

    return TCL_OK;
}

int Vtktcl_SafeInit(Tcl_Interp *interp)
{
  return Vtktcl_Init(interp);
}
