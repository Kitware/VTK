/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper2D.h
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
// .NAME vtkWin32PolyDataMapper2D - 2D PolyData support for windows
// .SECTION Description
// vtkWin32PolyDataMapper2D provides 2D PolyData annotation support for 
// vtk under windows.  Normally the user should use vtkPolyDataMapper2D 
// which in turn will use this class.

// .SECTION See Also
// vtkPolyDataMapper2D

#ifndef __vtkOpenGLPolyDataMapper2D_h
#define __vtkOpenGLPolyDataMapper2D_h

#include "vtkPolyDataMapper2D.h"

class VTK_RENDERING_EXPORT vtkOpenGLPolyDataMapper2D : public vtkPolyDataMapper2D
{
public:
  vtkTypeRevisionMacro(vtkOpenGLPolyDataMapper2D,vtkPolyDataMapper2D);
  static vtkOpenGLPolyDataMapper2D *New();

  // Description:
  // Actually draw the poly data.
  void RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor);

protected:
  vtkOpenGLPolyDataMapper2D() {};
  ~vtkOpenGLPolyDataMapper2D() {};
  
private:
  vtkOpenGLPolyDataMapper2D(const vtkOpenGLPolyDataMapper2D&);  // Not implemented.
  void operator=(const vtkOpenGLPolyDataMapper2D&);  // Not implemented.
};


#endif

