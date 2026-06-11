// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkOpenGLBatchedLabeledDataMapperInternals_h
#define vtkOpenGLBatchedLabeledDataMapperInternals_h

#include "vtkOpenGLPolyDataMapper.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLBatchedLabeledDataMapper;
VTK_ABI_NAMESPACE_END

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_NO_EXPORT vtkOpenGLBatchedLabeledDataMapperInternals
  : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLBatchedLabeledDataMapperInternals* New();
  vtkTypeMacro(vtkOpenGLBatchedLabeledDataMapperInternals, vtkOpenGLPolyDataMapper);

  vtkOpenGLBatchedLabeledDataMapper* Parent = nullptr;

protected:
  vtkOpenGLBatchedLabeledDataMapperInternals() = default;
  ~vtkOpenGLBatchedLabeledDataMapperInternals() override = default;

  void SetMapperShaderParameters(
    vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* actor) override;
  void BuildShaders(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor) override;

private:
  vtkOpenGLBatchedLabeledDataMapperInternals(
    const vtkOpenGLBatchedLabeledDataMapperInternals&) = delete;
  void operator=(const vtkOpenGLBatchedLabeledDataMapperInternals&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
