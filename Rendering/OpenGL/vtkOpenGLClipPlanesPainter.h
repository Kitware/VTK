/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLClipPlanesPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLClipPlanesPainter - painter that manages clipping
// .SECTION Description
// This painter is an openGL specific painter which handles clipplanes.
// This painter must typically be placed before the painter that
// do the primitive rendering.

#ifndef vtkOpenGLClipPlanesPainter_h
#define vtkOpenGLClipPlanesPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkClipPlanesPainter.h"

class vtkPlaneCollection;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLClipPlanesPainter : public vtkClipPlanesPainter
{
public:
  static vtkOpenGLClipPlanesPainter* New();
  vtkTypeMacro(vtkOpenGLClipPlanesPainter, vtkClipPlanesPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOpenGLClipPlanesPainter();
  ~vtkOpenGLClipPlanesPainter();

  // Description:
  // Generates rendering primitives of appropriate type(s).
  // Uses the clipping planes to set up clipping regions.
  // typeflags are ignored by this painter.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                              unsigned long typeflags, bool forceCompileOnly);
private:
  vtkOpenGLClipPlanesPainter(const vtkOpenGLClipPlanesPainter&); // Not implemented.
  void operator=(const vtkOpenGLClipPlanesPainter&); // Not implemented.
};

#endif
