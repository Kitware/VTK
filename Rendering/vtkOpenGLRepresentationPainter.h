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
// .NAME vtkOpenGLRepresentationPainter - painter handling representation 
// using OpenGL.
// .SECTION Description
// This is OpenGL implementation of a painter handling representation 
// i.e. Points, Wireframe, Surface.

#ifndef __vtkOpenGLRepresentationPainter_h
#define __vtkOpenGLRepresentationPainter_h

#include "vtkRepresentationPainter.h"
class vtkInformationIntegerKey;

class VTK_RENDERING_EXPORT vtkOpenGLRepresentationPainter : 
  public vtkRepresentationPainter
{
public:
  static vtkOpenGLRepresentationPainter* New();
  vtkTypeMacro(vtkOpenGLRepresentationPainter, vtkRepresentationPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This painter overrides GetTimeToDraw() to never pass the request to the
  // delegate. This is done since this class may propagate a single render
  // request multiple times to the delegate. In that case the time accumulation
  // responsibility is borne by the painter causing the multiple rendering
  // requests i.e. this painter itself.
  virtual double GetTimeToDraw()
    {
    return this->TimeToDraw;
    }

protected:
  vtkOpenGLRepresentationPainter();
  ~vtkOpenGLRepresentationPainter();

  // Description:
  // Changes the polygon mode according to the representation.
  void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
                      unsigned long typeflags,bool forceCompileOnly);
private:
  vtkOpenGLRepresentationPainter(const vtkOpenGLRepresentationPainter&); // Not implemented.
  void operator=(const vtkOpenGLRepresentationPainter&); // Not implemented.
};

#endif
