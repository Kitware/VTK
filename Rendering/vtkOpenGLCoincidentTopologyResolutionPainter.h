/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCoincidentTopologyResolutionPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLCoincidentTopologyResolutionPainter
// .SECTION Description
// Implementation for vtkCoincidentTopologyResolutionPainter using OpenGL.

#ifndef __vtkOpenGLCoincidentTopologyResolutionPainter_h
#define __vtkOpenGLCoincidentTopologyResolutionPainter_h

#include "vtkCoincidentTopologyResolutionPainter.h"

class VTK_RENDERING_EXPORT vtkOpenGLCoincidentTopologyResolutionPainter :
  public vtkCoincidentTopologyResolutionPainter
{
public:
  static vtkOpenGLCoincidentTopologyResolutionPainter* New();
  vtkTypeMacro(vtkOpenGLCoincidentTopologyResolutionPainter,
    vtkCoincidentTopologyResolutionPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOpenGLCoincidentTopologyResolutionPainter();
  ~vtkOpenGLCoincidentTopologyResolutionPainter();

  // Description:
  // Performs the actual rendering. Subclasses may override this method.
  // default implementation merely call a Render on the DelegatePainter,
  // if any. When RenderInternal() is called, it is assured that the 
  // DelegatePainter is in sync with this painter i.e. UpdatePainter()
  // has been called.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
                              unsigned long typeflags, bool forceCompileOnly);

  void RenderPolygonOffset(vtkRenderer *renderer, vtkActor *actor,
    unsigned long typeflags, bool forceCompileOnly);
  void RenderShiftZBuffer(vtkRenderer *renderer, vtkActor *actor,
    unsigned long typeflags, bool forceCompileOnly);

private:
  vtkOpenGLCoincidentTopologyResolutionPainter(
    const vtkOpenGLCoincidentTopologyResolutionPainter&); // Not implemented.
  void operator=(const vtkOpenGLCoincidentTopologyResolutionPainter&); // Not implemented.
};


#endif
