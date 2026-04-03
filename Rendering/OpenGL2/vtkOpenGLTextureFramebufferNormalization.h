// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkOpenGLTextureFramebufferNormalization
 * @brief Framebuffer-based shader conversion for 16-bit to float texture normalization
 *
 * Uses a framebuffer with a fragment shader to convert 16-bit integer texture data
 * to normalized 32-bit float. This is the preferred GPU path when compute shaders
 * are unavailable (e.g., on GLES 3.0 without GL_ARB_compute_shader).
 *
 * Strategy:
 * 1. Upload integer data to a temporary R16UI/R16I texture
 * 2. Use a shader program that samples from R16UI and writes to R32F
 * 3. Render a fullscreen quad to perform the conversion
 *
 * Call Initialize() after New() to bind the helper to an OpenGL context.
 * If initialization fails, GetConversionMode() returns Unsupported.
 *
 * @sa vtkOpenGLTextureNormalizationHelper, vtkOpenGLTextureCPUNormalization,
 *     vtkOpenGLTextureComputeShaderNormalization
 */

#ifndef vtkOpenGLTextureFramebufferNormalization_h
#define vtkOpenGLTextureFramebufferNormalization_h

#include "vtkOpenGLTextureNormalizationHelper.h"
#include "vtkRenderingOpenGL2Module.h"
#include "vtk_glad.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLTextureFramebufferNormalization
  : public vtkOpenGLTextureNormalizationHelper
{
public:
  static vtkOpenGLTextureFramebufferNormalization* New();
  vtkTypeMacro(vtkOpenGLTextureFramebufferNormalization, vtkOpenGLTextureNormalizationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the framebuffer conversion helper with the given render window context.
   * Must be called after New() before using the helper.
   * Sets Mode to Unsupported if initialization fails.
   */
  void Initialize(vtkOpenGLRenderWindow* context);

  bool ConvertUShortToFloat(const void* sourceData, size_t numValues, int numComps,
    unsigned int targetTexture, unsigned int width, unsigned int height) override;

  bool ConvertShortToFloat(const void* sourceData, size_t numValues, int numComps,
    unsigned int targetTexture, unsigned int width, unsigned int height) override;

protected:
  vtkOpenGLTextureFramebufferNormalization();
  ~vtkOpenGLTextureFramebufferNormalization() override;

  vtkOpenGLRenderWindow* Context = nullptr;
  GLuint ConversionProgram = 0;
  GLuint IntermediateTexture = 0;
  GLuint ConversionFramebuffer = 0;
  GLuint ConversionVAO = 0;

  bool InitializeConversionShader();
  bool UploadToIntermediateTexture(const void* sourceData, size_t numValues, int numComps,
    unsigned int width, unsigned int height, GLenum internalFormat, GLenum dataFormat);
  bool PerformFramebufferConversion(unsigned int targetTexture, int numComps, unsigned int width,
    unsigned int height, float scaleFactor);

private:
  vtkOpenGLTextureFramebufferNormalization(
    const vtkOpenGLTextureFramebufferNormalization&) = delete;
  void operator=(const vtkOpenGLTextureFramebufferNormalization&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLTextureFramebufferNormalization_h
