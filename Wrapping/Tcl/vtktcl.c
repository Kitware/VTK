/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtktcl.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToolkits.h"

#ifdef VTK_USE_RENDERING
# include "vtkTk.h"
#else
# include "vtkTcl.h"
#endif

extern int Vtkcommontcl_Init(Tcl_Interp *interp);
extern int Vtkfilteringtcl_Init(Tcl_Interp *interp);
extern int Vtkgraphicstcl_Init(Tcl_Interp *interp);
extern int Vtkimagingtcl_Init(Tcl_Interp *interp);
extern int Vtkiotcl_Init(Tcl_Interp *interp);

#ifdef VTK_USE_RENDERING
extern int Vtkrenderingtcl_Init(Tcl_Interp *interp);
extern int Vtkvolumerenderingtcl_Init(Tcl_Interp *interp);
extern int Vtkhybridtcl_Init(Tcl_Interp *interp);
extern int Vtkwidgetstcl_Init(Tcl_Interp *interp);
#ifdef VTK_USE_TKWIDGET
extern int Vtktkrenderwidget_Init(Tcl_Interp *interp);
extern int Vtktkimagewindowwidget_Init(Tcl_Interp *interp);
extern int Vtktkimageviewerwidget_Init(Tcl_Interp *interp);
#endif
#endif

#ifdef VTK_USE_PARALLEL
extern int Vtkparalleltcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_GEOVIS
extern int Vtkgeovistcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_INFOVIS
extern int Vtkinfovistcl_Init(Tcl_Interp *interp);
#endif

#ifdef VTK_USE_VIEWS
extern int Vtkviewstcl_Init(Tcl_Interp *interp);
#endif

int Vtktcl_Init(Tcl_Interp *interp)
{
  /* init the core vtk stuff */
  if (Vtkcommontcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
  if (Vtkfilteringtcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
  if (Vtkiotcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
  if (Vtkgraphicstcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }
  if (Vtkimagingtcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }

#ifdef VTK_USE_RENDERING
  if (Vtkrenderingtcl_Init(interp) == TCL_ERROR)
    {
    return TCL_ERROR;
    }

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

    return TCL_OK;
}

int Vtktcl_SafeInit(Tcl_Interp *interp)
{
  return Vtktcl_Init(interp);
}
