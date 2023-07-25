// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBatchedSurfaceLICMapper
 * @brief   Implements batched rendering of multiple vtkPolyData using LIC interface and OpenGL.
 *
 * @sa vtkCompositeSurfaceLICMapperDelegator
 */

#ifndef vtkBatchedSurfaceLICMapper_h
#define vtkBatchedSurfaceLICMapper_h

#include "vtkOpenGLBatchedPolyDataMapper.h"

#include "vtkRenderingLICOpenGL2Module.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGLICOPENGL2_EXPORT vtkBatchedSurfaceLICMapper
  : public vtkOpenGLBatchedPolyDataMapper
{
public:
  static vtkBatchedSurfaceLICMapper* New();
  vtkTypeMacro(vtkBatchedSurfaceLICMapper, vtkOpenGLBatchedPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkBatchedSurfaceLICMapper();
  ~vtkBatchedSurfaceLICMapper() override;

  /**
   * Build the VBO/IBO, called by UpdateBufferObjects
   */
  void AppendOneBufferObject(vtkRenderer* ren, vtkActor* act, GLBatchElement* glBatchElement,
    vtkIdType& flat_index, std::vector<unsigned char>& colors, std::vector<float>& norms) override;

  /**
   * Set the shader parameters related to the mapper/input data, called by UpdateShader
   */
  void SetMapperShaderParameters(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

  /**
   * Perform string replacements on the shader templates
   */
  void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;

private:
  vtkBatchedSurfaceLICMapper(const vtkBatchedSurfaceLICMapper&) = delete;
  void operator=(const vtkBatchedSurfaceLICMapper&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
