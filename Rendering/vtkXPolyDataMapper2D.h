/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXPolyDataMapper2D.h
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
// .NAME vtkXPolyDataMapper2D - 2D PolyData support for windows
// .SECTION Description
// vtkXPolyDataMapper2D provides 2D PolyData annotation support for 
// vtk under windows.  Normally the user should use vtkPolyDataMapper2D 
// which in turn will use this class.

// .SECTION See Also
// vtkPolyDataMapper2D

#ifndef __vtkXPolyDataMapper2D_h
#define __vtkXPolyDataMapper2D_h

#include "vtkPolyDataMapper2D.h"

#ifndef VTK_REMOVE_LEGACY_CODE
class VTK_RENDERING_EXPORT vtkXPolyDataMapper2D : public vtkPolyDataMapper2D
{
public:
  vtkTypeRevisionMacro(vtkXPolyDataMapper2D,vtkPolyDataMapper2D);
  static vtkXPolyDataMapper2D *New();

  // Description:
  // Actually draw the poly data.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

protected:
  vtkXPolyDataMapper2D() {};
  ~vtkXPolyDataMapper2D() {};
  
private:
  vtkXPolyDataMapper2D(const vtkXPolyDataMapper2D&);  // Not implemented.
  void operator=(const vtkXPolyDataMapper2D&);  // Not implemented.
};
#endif

#endif

