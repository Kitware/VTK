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
/**
 * @class   vtkOpenGLRepresentationPainter
 * @brief   painter handling representation
 * using OpenGL.
 *
 * This is OpenGL implementation of a painter handling representation
 * i.e. Points, Wireframe, Surface.
*/

#ifndef vtkOpenGLRepresentationPainter_h
#define vtkOpenGLRepresentationPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRepresentationPainter.h"
class vtkInformationIntegerKey;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLRepresentationPainter :
  public vtkRepresentationPainter
{
public:
  static vtkOpenGLRepresentationPainter* New();
  vtkTypeMacro(vtkOpenGLRepresentationPainter, vtkRepresentationPainter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * This painter overrides GetTimeToDraw() to never pass the request to the
   * delegate. This is done since this class may propagate a single render
   * request multiple times to the delegate. In that case the time accumulation
   * responsibility is borne by the painter causing the multiple rendering
   * requests i.e. this painter itself.
   */
  double GetTimeToDraw() VTK_OVERRIDE
  {
    return this->TimeToDraw;
  }

protected:
  vtkOpenGLRepresentationPainter();
  ~vtkOpenGLRepresentationPainter() VTK_OVERRIDE;

  /**
   * Changes the polygon mode according to the representation.
   */
  void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                      unsigned long typeflags,bool forceCompileOnly) VTK_OVERRIDE;

private:
  vtkOpenGLRepresentationPainter(const vtkOpenGLRepresentationPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLRepresentationPainter&) VTK_DELETE_FUNCTION;
};

#endif
