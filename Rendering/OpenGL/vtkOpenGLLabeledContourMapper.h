/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLabeledContourMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLLabeledContourMapper
// .SECTION Description
// vtkOpenGLLabeledContourMapper is an override for vtkLabeledContourMapper
// that implements stenciling using the OpenGL API.

#ifndef __vtkOpenGLLabelContourMapper_h
#define __vtkOpenGLLabelContourMapper_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkLabeledContourMapper.h"

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLLabeledContourMapper
    : public vtkLabeledContourMapper
{
public:
  static vtkOpenGLLabeledContourMapper *New();
  vtkTypeMacro(vtkOpenGLLabeledContourMapper, vtkLabeledContourMapper)

protected:
  vtkOpenGLLabeledContourMapper();
  ~vtkOpenGLLabeledContourMapper();

  bool ApplyStencil(vtkRenderer *ren, vtkActor *act);
  bool RemoveStencil();

private:
  vtkOpenGLLabeledContourMapper(const vtkOpenGLLabeledContourMapper&);  // Not implemented.
  void operator=(const vtkOpenGLLabeledContourMapper&);  // Not implemented.

  void DrawFullScreenQuad(vtkRenderer *ren);
};

#endif
