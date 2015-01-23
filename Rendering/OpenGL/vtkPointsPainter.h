/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointsPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointsPainter - this painter paints verts.
// .SECTION Description
// This painter tries to paint points efficiently. Request to Render
// any other primitive are ignored and not passed to the delegate painter,
// if any. This painter cannot handle cell colors/normals. If they are
// present the request is passed on to the Delegate painter. If this
// class is able to render the primitive, the render request is not
// propagated to the delegate painter.
#ifndef vtkPointsPainter_h
#define vtkPointsPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPrimitivePainter.h"

class VTKRENDERINGOPENGL_EXPORT vtkPointsPainter : public vtkPrimitivePainter
{
public:
  static vtkPointsPainter* New();
  vtkTypeMacro(vtkPointsPainter, vtkPrimitivePainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPointsPainter();
  ~vtkPointsPainter();

  // Description:
  // The actual rendering happens here. This method is called only when
  // SupportedPrimitive is present in typeflags when Render() is invoked.
  virtual int RenderPrimitive(unsigned long flags, vtkDataArray* n,
    vtkUnsignedCharArray* c, vtkDataArray* t, vtkRenderer* ren);
private:
  vtkPointsPainter(const vtkPointsPainter&); // Not implemented.
  void operator=(const vtkPointsPainter&); // Not implemented.
};

#endif
