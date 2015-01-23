/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCompositePainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLCompositePainter - composite painter for OpenGL.
// .SECTION Description

#ifndef vtkOpenGLCompositePainter_h
#define vtkOpenGLCompositePainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkCompositePainter.h"

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLCompositePainter : public vtkCompositePainter
{
public:
  static vtkOpenGLCompositePainter* New();
  vtkTypeMacro(vtkOpenGLCompositePainter, vtkCompositePainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOpenGLCompositePainter();
  ~vtkOpenGLCompositePainter();

  // Description:
  // Overridden in vtkOpenGLCompositePainter to pass attributes to OpenGL.
  virtual void UpdateRenderingState(
    vtkRenderWindow* window, vtkProperty* property, RenderBlockState& state);

private:
  vtkOpenGLCompositePainter(const vtkOpenGLCompositePainter&); // Not implemented.
  void operator=(const vtkOpenGLCompositePainter&); // Not implemented.

  bool PushedOpenGLAttribs;
};

#endif
