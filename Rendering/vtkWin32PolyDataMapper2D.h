/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32PolyDataMapper2D.h
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
// .NAME vtkWin32PolyDataMapper2D - (obsolete) 2D PolyData support for windows
// .SECTION Description
// vtkWin32PolyDataMapper2D provides 2D PolyData annotation support for 
// vtk under windows.  Normally the user should use vtkPolyDataMapper2D 
// which in turn will use this class.

// .SECTION See Also
// vtkPolyDataMapper2D

#ifndef __vtkWin32PolyDataMapper2D_h
#define __vtkWin32PolyDataMapper2D_h

#include "vtkPolyDataMapper2D.h"

#ifndef VTK_REMOVE_LEGACY_CODE
class VTK_RENDERING_EXPORT vtkWin32PolyDataMapper2D : public vtkPolyDataMapper2D
{
public:
  vtkTypeRevisionMacro(vtkWin32PolyDataMapper2D,vtkPolyDataMapper2D);
  static vtkWin32PolyDataMapper2D *New();

  // Description:
  // Actually draw the poly data.
  virtual void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

protected:
  vtkWin32PolyDataMapper2D() {};
  ~vtkWin32PolyDataMapper2D() {};
  
private:
  vtkWin32PolyDataMapper2D(const vtkWin32PolyDataMapper2D&);  // Not implemented.
  void operator=(const vtkWin32PolyDataMapper2D&);  // Not implemented.
};
#endif

#endif

