// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLSphereMapper
 * @brief   draw spheres using imposters
 *
 * An OpenGL mapper that uses imposters to draw spheres. Supports
 * transparency and picking as well.
 *
 * Every point of the input is drawn as a sphere, regardless of the cells in the input.
 * Each point is drawn as an instanced, camera-facing quad (a 4-vertex triangle strip)
 * that is expanded in the vertex shader. The fragment shader ray-casts the sphere
 * within that quad to compute the normal and depth of every fragment.
 * No geometry shader is required, so this mapper also works with OpenGL ES 3.0 and WebGL2.
 */

#ifndef vtkOpenGLSphereMapper_h
#define vtkOpenGLSphereMapper_h

#include "vtkOpenGLLowMemoryPolyDataMapper.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLSphereMapper
  : public vtkOpenGLLowMemoryPolyDataMapper
{
public:
  static vtkOpenGLSphereMapper* New();
  vtkTypeMacro(vtkOpenGLSphereMapper, vtkOpenGLLowMemoryPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Convenience method to set the array to scale with.
   * The first component of this point data array is the radius of each sphere.
   */
  vtkSetStringMacro(ScaleArray);
  vtkGetStringMacro(ScaleArray);
  ///@}

  ///@{
  /**
   * This value will be used for the radius is the scale
   * array is not provided.
   */
  vtkSetMacro(Radius, float);
  vtkGetMacro(Radius, float);
  ///@}

  /**
   * This calls RenderPiece (twice when transparent)
   */
  void Render(vtkRenderer* ren, vtkActor* act) override;

protected:
  vtkOpenGLSphereMapper();
  ~vtkOpenGLSphereMapper() override;

  /**
   * Upload the scale array as a texture buffer in addition to the arrays
   * uploaded by the superclass.
   */
  bool BindArraysToTextureBuffers(vtkRenderer* renderer, vtkActor* actor,
    vtkCellGraphicsPrimitiveMap::CellTypeMapperOffsets& offsets) override;
  void InstallArrayTextureShaderDeclarations() override;

  /**
   * Place the corners of the quad around each sphere.
   */
  void ReplaceShaderPosition(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource) override;

  /**
   * Ray-cast the sphere to compute the normal and depth of each fragment.
   */
  void ReplaceShaderNormal(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource) override;

  void SetShaderParameters(vtkRenderer* renderer, vtkActor* actor) override;

  char* ScaleArray = nullptr;
  float Radius = 0.3f;
  // used for transparency
  bool Invert = false;
  // whether the scale array was uploaded in the last call to BindArraysToTextureBuffers
  bool HasScaleArray = false;

private:
  vtkOpenGLSphereMapper(const vtkOpenGLSphereMapper&) = delete;
  void operator=(const vtkOpenGLSphereMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
