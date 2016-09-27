/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinesPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLinesPainter
 * @brief   painter that paints lines.
 *
 * This painter tries to paint lines efficiently. Request to Render
 * any other primitive are ignored and not passed to the delegate painter,
 * if any. This painter cannot handle cell colors/normals. If they are
 * present the request is passed on to the Delegate painter. If this
 * class is able to render the primitive, the render request is not
 * propagated to the delegate painter.
 *
*/

#ifndef vtkLinesPainter_h
#define vtkLinesPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPrimitivePainter.h"

class VTKRENDERINGOPENGL_EXPORT vtkLinesPainter : public vtkPrimitivePainter
{
public:
  static vtkLinesPainter* New();
  vtkTypeMacro(vtkLinesPainter, vtkPrimitivePainter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkLinesPainter();
  ~vtkLinesPainter();

  int RenderPolys; // Flag indicating if the line loops are to be closed.

  /**
   * Overriden to set RenderPolys flag. When set, polys are rendered
   * as line loops.
   */
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                              unsigned long typeflags, bool forceCompileOnly);

  /**
   * The actual rendering happens here. This method is called only when
   * SupportedPrimitive is present in typeflags when Render() is invoked.
   */
  virtual int RenderPrimitive(unsigned long flags, vtkDataArray* n,
    vtkUnsignedCharArray* c, vtkDataArray* t, vtkRenderer* ren);

private:
  vtkLinesPainter(const vtkLinesPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLinesPainter&) VTK_DELETE_FUNCTION;
};



#endif
