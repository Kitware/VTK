/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaClipPlanesPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaClipPlanesPainter - painter that manages clipping
// .SECTION Description
// This painter is an openGL specific painter which handles clipplanes.
// This painter must typically be placed before the painter that 
// do the primitive rendering.

#ifndef __vtkMesaClipPlanesPainter_h
#define __vtkMesaClipPlanesPainter_h

#include "vtkClipPlanesPainter.h"

class vtkPlaneCollection;

class VTK_RENDERING_EXPORT vtkMesaClipPlanesPainter : public vtkClipPlanesPainter
{
public:
  static vtkMesaClipPlanesPainter* New();
  vtkTypeMacro(vtkMesaClipPlanesPainter, vtkClipPlanesPainter);
  void PrintSelf(ostream& os ,vtkIndent indent);

protected:
  vtkMesaClipPlanesPainter();
  ~vtkMesaClipPlanesPainter();

  // Description:
  // Generates rendering primitives of appropriate type(s).
  // Uses the clipping planes to set up clipping regions.
  // typeflags are ignored by this painter.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
    unsigned long typeflags);
private:
  vtkMesaClipPlanesPainter(const vtkMesaClipPlanesPainter&); // Not implemented.
  void operator=(const vtkMesaClipPlanesPainter&); // Not implemented.
};



#endif

