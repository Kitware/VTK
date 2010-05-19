/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaPolyDataMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaPolyDataMapper2D - 2D PolyData support for Mesa
// .SECTION Description
// vtkMesaPolyDataMapper2D provides 2D PolyData annotation support for 
// vtk under Mesa.  Normally the user should use vtkPolyDataMapper2D 
// which in turn will use this class.

// .SECTION See Also
// vtkPolyDataMapper2D

#ifndef __vtkMesaPolyDataMapper2D_h
#define __vtkMesaPolyDataMapper2D_h

#include "vtkPolyDataMapper2D.h"

class VTK_RENDERING_EXPORT vtkMesaPolyDataMapper2D : public vtkPolyDataMapper2D
{
public:
  vtkTypeMacro(vtkMesaPolyDataMapper2D,vtkPolyDataMapper2D);
  static vtkMesaPolyDataMapper2D *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Actually draw the poly data.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

protected:
  vtkMesaPolyDataMapper2D() {};
  ~vtkMesaPolyDataMapper2D() {};
  
private:
  vtkMesaPolyDataMapper2D(const vtkMesaPolyDataMapper2D&);  // Not implemented.
  void operator=(const vtkMesaPolyDataMapper2D&);  // Not implemented.
};


#endif

