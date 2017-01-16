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
/**
 * @class   vtkOpenGLLabeledContourMapper
 *
 * vtkOpenGLLabeledContourMapper is an override for vtkLabeledContourMapper
 * that implements stenciling using the OpenGL API.
*/

#ifndef vtkOpenGLLabeledContourMapper_h
#define vtkOpenGLLabeledContourMapper_h

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
  ~vtkOpenGLLabeledContourMapper() VTK_OVERRIDE;

  bool ApplyStencil(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;
  bool RemoveStencil() VTK_OVERRIDE;

private:
  vtkOpenGLLabeledContourMapper(const vtkOpenGLLabeledContourMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLLabeledContourMapper&) VTK_DELETE_FUNCTION;
};

#endif
