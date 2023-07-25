// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLHyperTreeGridMapper
 * @brief   map vtkHyperTreeGrid to graphics primitives
 *
 * vtkOpenGLHyperTreeGridMapper is a class that uses OpenGL to do the actual
 * rendering of Hyper Tree Grid.
 */

#ifndef vtkOpenGLHyperTreeGridMapper_h
#define vtkOpenGLHyperTreeGridMapper_h

#include "vtkHyperTreeGridMapper.h"
#include "vtkSetGet.h"       // Get macro
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include "vtkRenderingOpenGL2Module.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLHyperTreeGridMapper : public vtkHyperTreeGridMapper
{
public:
  static vtkOpenGLHyperTreeGridMapper* New();
  vtkTypeMacro(vtkOpenGLHyperTreeGridMapper, vtkHyperTreeGridMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkOpenGLHyperTreeGridMapper();
  ~vtkOpenGLHyperTreeGridMapper() override = default;

private:
  vtkOpenGLHyperTreeGridMapper(const vtkOpenGLHyperTreeGridMapper&) = delete;
  void operator=(const vtkOpenGLHyperTreeGridMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
