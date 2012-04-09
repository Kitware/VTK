/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLightingPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLLightingPainter - painter that manages lighting.
// .SECTION Description
// This painter manages lighting.
// Ligting is disabled when rendering points/lines and no normals are present
// or rendering Polygons/TStrips and representation is points and no normals
// are present.

#ifndef __vtkOpenGLLightingPainter_h
#define __vtkOpenGLLightingPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkLightingPainter.h"

class vtkWindow;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLLightingPainter : public vtkLightingPainter
{
public:
  static vtkOpenGLLightingPainter* New();
  vtkTypeMacro(vtkOpenGLLightingPainter, vtkLightingPainter);
  void PrintSelf(ostream& os ,vtkIndent indent);

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
  vtkOpenGLLightingPainter();
  ~vtkOpenGLLightingPainter();

  // Description:
  // Setups lighting state before calling render on delegate
  // painter.
  virtual void RenderInternal(vtkRenderer *renderer,
                              vtkActor *actor,
                              unsigned long typeflags,
                              bool forceCompileOnly);

private:
  vtkOpenGLLightingPainter(const vtkOpenGLLightingPainter&); // Not implemented.
  void operator=(const vtkOpenGLLightingPainter&); // Not implemented.
};

#endif

