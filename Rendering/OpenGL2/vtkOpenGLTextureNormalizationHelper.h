// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkOpenGLTextureNormalizationHelper
 * @brief GPU-based texture normalization for GLES 3.0 without GL_EXT_texture_norm16
 *
 * This helper provides zero-copy conversion of 16-bit integer texture data
 * to normalized float format using GPU-assisted operations on GLES 3.0.
 *
 * When GL_EXT_texture_norm16 is unavailable on GLES 3.0, this helper converts
 * 16-bit integer texture data to normalized 32-bit float format using GPU operations
 * instead of CPU conversion, eliminating the memory overhead of temporary float vectors.
 *
 * Supported conversions:
 *  - VTK_UNSIGNED_SHORT -> GL_R32F (dividing by 65535.0)
 *  - VTK_SHORT -> GL_R32F (dividing by 32767.0)
 *  - Multi-channel variants (RG32F, RGB32F, RGBA32F)
 *
 * Use Create() to obtain a concrete subclass appropriate for the current OpenGL context.
 * Concrete implementations are:
 *  - vtkOpenGLTextureCPUNormalization: CPU-based fallback
 *  - vtkOpenGLTextureComputeShaderNormalization: GPU compute shader (GL 4.3+)
 *  - vtkOpenGLTextureFramebufferNormalization: Framebuffer-based shader conversion
 */

#ifndef vtkOpenGLTextureNormalizationHelper_h
#define vtkOpenGLTextureNormalizationHelper_h

#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h"           // For ivar
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLTextureNormalizationHelper
  : public vtkObject
{
public:
  vtkAbstractTypeMacro(vtkOpenGLTextureNormalizationHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum class ConversionMode
  {
    CPU,           // CPU conversion (fallback)
    ComputeShader, // Compute shader (fastest if available)
    CopyTexImage,  // Framebuffer blit with shader conversion
    Unsupported    // No conversion available
  };

  /**
   * Create a helper instance appropriate for the given OpenGL context.
   * Selects the best available implementation based on OpenGL capabilities.
   */
  static vtkSmartPointer<vtkOpenGLTextureNormalizationHelper> Create(
    vtkOpenGLRenderWindow* context);

  /**
   * Get the preferred conversion mode for this context
   */
  ConversionMode GetConversionMode() const { return this->Mode; }

  /**
   * Check if GPU-assisted conversion is available
   */
  bool IsGPUConversionAvailable() const
  {
    return this->Mode != ConversionMode::CPU && this->Mode != ConversionMode::Unsupported;
  }

  /**
   * Convert unsigned short texture data to float in GPU memory
   *
   * @param sourceData Pointer to VTK_UNSIGNED_SHORT data
   * @param numValues Total number of short values
   * @param numComps Number of components (1, 2, 3, or 4)
   * @param targetTexture Handle to target GL_R32F/GL_RG32F/etc texture
   * @param width Texture width
   * @param height Texture height
   * @return true if conversion succeeded
   */
  virtual bool ConvertUShortToFloat(const void* sourceData, size_t numValues, int numComps,
    unsigned int targetTexture, unsigned int width, unsigned int height) = 0;

  /**
   * Convert signed short texture data to float in GPU memory
   *
   * @param sourceData Pointer to VTK_SHORT data
   * @param numValues Total number of short values
   * @param numComps Number of components (1, 2, 3, or 4)
   * @param targetTexture Handle to target GL_R32F/GL_RG32F/etc texture
   * @param width Texture width
   * @param height Texture height
   * @return true if conversion succeeded
   */
  virtual bool ConvertShortToFloat(const void* sourceData, size_t numValues, int numComps,
    unsigned int targetTexture, unsigned int width, unsigned int height) = 0;

protected:
  vtkOpenGLTextureNormalizationHelper() = default;
  ~vtkOpenGLTextureNormalizationHelper() override = default;

  ConversionMode Mode = ConversionMode::CPU;

private:
  vtkOpenGLTextureNormalizationHelper(const vtkOpenGLTextureNormalizationHelper&) = delete;
  void operator=(const vtkOpenGLTextureNormalizationHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLTextureNormalizationHelper_h
