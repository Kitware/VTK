/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtktcl.c
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/


#include "tk.h"

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

    return TCL_OK;
}

int Vtktcl_SafeInit(Tcl_Interp *interp)
{
  return Vtktcl_Init(interp);
}
