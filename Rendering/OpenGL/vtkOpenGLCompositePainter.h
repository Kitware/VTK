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
/**
 * @class   vtkOpenGLCompositePainter
 * @brief   composite painter for OpenGL.
 *
*/

#ifndef vtkOpenGLCompositePainter_h
#define vtkOpenGLCompositePainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkCompositePainter.h"

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLCompositePainter : public vtkCompositePainter
{
public:
  static vtkOpenGLCompositePainter* New();
  vtkTypeMacro(vtkOpenGLCompositePainter, vtkCompositePainter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkOpenGLCompositePainter();
  ~vtkOpenGLCompositePainter() override;

  /**
   * Overridden in vtkOpenGLCompositePainter to pass attributes to OpenGL.
   */
  void UpdateRenderingState(
    vtkRenderWindow* window, vtkProperty* property, RenderBlockState& state) override;

private:
  vtkOpenGLCompositePainter(const vtkOpenGLCompositePainter&) = delete;
  void operator=(const vtkOpenGLCompositePainter&) = delete;

  bool PushedOpenGLAttribs;
};

#endif
