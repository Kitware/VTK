/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtktcl.c
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
