/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRepresentationPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkOpenGLRepresentationPainter_h
#define __vtkOpenGLRepresentationPainter_h

#include "vtkRepresentationPainter.h"

class VTK_RENDERING_EXPORT vtkOpenGLRepresentationPainter : 
  public vtkRepresentationPainter
{
public:
  static vtkOpenGLRepresentationPainter* New();
  vtkTypeRevisionMacro(vtkOpenGLRepresentationPainter, vtkRepresentationPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOpenGLRepresentationPainter();
  ~vtkOpenGLRepresentationPainter();

  // Description:
  // Changes the polygon mode according to the representation.
  void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
    unsigned long typeflags);
private:
  vtkOpenGLRepresentationPainter(const vtkOpenGLRepresentationPainter&); // Not implemented.
  void operator=(const vtkOpenGLRepresentationPainter&); // Not implemented.
};

#endif
