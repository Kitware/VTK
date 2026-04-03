// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkOpenGLTextureComputeShaderNormalization
 * @brief GPU compute shader implementation for 16-bit to float texture normalization
 *
 * Uses OpenGL compute shaders (GL 4.3+ or ARB_compute_shader) to perform
 * GPU-accelerated normalization of 16-bit integer texture data to 32-bit float.
 * This is the fastest available implementation when compute shaders are supported.
 *
 * Call Initialize() after New() to bind the helper to an OpenGL context.
 * If initialization fails, GetConversionMode() returns Unsupported.
 *
 * @sa vtkOpenGLTextureNormalizationHelper, vtkOpenGLTextureCPUNormalization,
 *     vtkOpenGLTextureFramebufferNormalization
 */

#ifndef vtkOpenGLTextureComputeShaderNormalization_h
#define vtkOpenGLTextureComputeShaderNormalization_h

#include "vtkOpenGLTextureNormalizationHelper.h"
#include "vtkRenderingOpenGL2Module.h"
#include "vtk_glad.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

#ifdef GL_COMPUTE_SHADER

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLTextureComputeShaderNormalization
  : public vtkOpenGLTextureNormalizationHelper
{
public:
  static vtkOpenGLTextureComputeShaderNormalization* New();
  vtkTypeMacro(vtkOpenGLTextureComputeShaderNormalization, vtkOpenGLTextureNormalizationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the compute shader with the given render window context.
   * Must be called after New() before using the helper.
   * Sets Mode to Unsupported if initialization fails.
   */
  void Initialize(vtkOpenGLRenderWindow* context);

  bool ConvertUShortToFloat(const void* sourceData, size_t numValues, int numComps,
    unsigned int targetTexture, unsigned int width, unsigned int height) override;

  bool ConvertShortToFloat(const void* sourceData, size_t numValues, int numComps,
    unsigned int targetTexture, unsigned int width, unsigned int height) override;

protected:
  vtkOpenGLTextureComputeShaderNormalization();
  ~vtkOpenGLTextureComputeShaderNormalization() override;

  vtkOpenGLRenderWindow* Context = nullptr;
  GLuint ComputeProgram = 0;
  GLuint IntermediateStorageTexture = 0;

  bool InitializeComputeProgram();
  bool UploadToIntermediateTexture(const void* sourceData, size_t numValues, int numComps,
    unsigned int width, unsigned int height, GLenum sourceFormat);
  bool RunNormalizationCompute(
    int numComps, unsigned int targetTexture, unsigned int width, unsigned int height);

private:
  vtkOpenGLTextureComputeShaderNormalization(
    const vtkOpenGLTextureComputeShaderNormalization&) = delete;
  void operator=(const vtkOpenGLTextureComputeShaderNormalization&) = delete;
};

#endif // GL_COMPUTE_SHADER

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLTextureComputeShaderNormalization_h
