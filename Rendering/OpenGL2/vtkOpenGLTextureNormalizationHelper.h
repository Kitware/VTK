/**
 * @file vtkOpenGLTextureNormalizationHelper.h
 * @brief GPU-based texture normalization for GLES 3.0 without GL_EXT_texture_norm16
 *
 * This helper provides zero-copy conversion of 16-bit integer texture data
 * to normalized float format using GPU-assisted operations on GLES 3.0.
 */

#ifndef vtkOpenGLTextureNormalizationHelper_h
#define vtkOpenGLTextureNormalizationHelper_h

#include "vtkRenderingOpenGL2Module.h"
#include "vtk_glad.h"
#include <memory>

class vtkOpenGLRenderWindow;

/**
 * GPU-based texture normalization helper for GLES 3.0 without norm16 extension
 *
 * When GL_EXT_texture_norm16 is unavailable on GLES 3.0, this helper converts
 * 16-bit integer texture data to normalized 32-bit float format using GPU operations
 * instead of CPU conversion, eliminating the memory overhead of temporary float vectors.
 *
 * Supported conversions:
 *  - VTK_UNSIGNED_SHORT -> GL_R32F (dividing by 65535.0)
 *  - VTK_SHORT -> GL_R32F (dividing by 32767.0)
 *  - Multi-channel variants (RG32F, RGB32F, RGBA32F)
 */
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLTextureNormalizationHelper
{
public:
  enum class ConversionMode
  {
    CPU,           // CPU conversion (fallback)
    ComputeShader, // Compute shader (fastest if available)
    CopyTexImage,  // Framebuffer blit with shader conversion
    Unsupported    // No conversion available
  };

  /**
   * Create a helper instance for the given OpenGL context
   */
  static std::shared_ptr<vtkOpenGLTextureNormalizationHelper> Create(
    vtkOpenGLRenderWindow* context);

  virtual ~vtkOpenGLTextureNormalizationHelper() = default;

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
    GLuint targetTexture, unsigned int width, unsigned int height) = 0;

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
    GLuint targetTexture, unsigned int width, unsigned int height) = 0;

protected:
  vtkOpenGLTextureNormalizationHelper(ConversionMode mode)
    : Mode(mode)
  {
  }

  ConversionMode Mode;
};

/**
 * CPU-based fallback implementation (current approach)
 */
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLTextureCPUNormalization
  : public vtkOpenGLTextureNormalizationHelper
{
public:
  vtkOpenGLTextureCPUNormalization()
    : vtkOpenGLTextureNormalizationHelper(ConversionMode::CPU)
  {
  }

  bool ConvertUShortToFloat(const void* sourceData, size_t numValues, int numComps,
    GLuint targetTexture, unsigned int width, unsigned int height) override;

  bool ConvertShortToFloat(const void* sourceData, size_t numValues, int numComps,
    GLuint targetTexture, unsigned int width, unsigned int height) override;

private:
  // Reusable buffer to avoid allocations
  mutable std::vector<float> ConversionBuffer;
};

#ifdef GL_COMPUTE_SHADER
/**
 * GPU compute shader implementation (if compute shaders available)
 */
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLTextureComputeShaderNormalization
  : public vtkOpenGLTextureNormalizationHelper
{
public:
  vtkOpenGLTextureComputeShaderNormalization(vtkOpenGLRenderWindow* context);
  ~vtkOpenGLTextureComputeShaderNormalization() override;

  bool ConvertUShortToFloat(const void* sourceData, size_t numValues, int numComps,
    GLuint targetTexture, unsigned int width, unsigned int height) override;

  bool ConvertShortToFloat(const void* sourceData, size_t numValues, int numComps,
    GLuint targetTexture, unsigned int width, unsigned int height) override;

private:
  vtkOpenGLRenderWindow* Context = nullptr;
  GLuint ComputeProgram = 0;
  GLuint IntermediateStorageTexture = 0;

  bool InitializeComputeProgram();
  bool UploadToIntermediateTexture(const void* sourceData, size_t numValues, int numComps,
    unsigned int width, unsigned int height, GLenum sourceFormat);
  bool RunNormalizationCompute(
    int numComps, GLuint targetTexture, unsigned int width, unsigned int height);
};
#endif // GL_COMPUTE_SHADER

/**
 * Framebuffer-based shader conversion (fallback for devices without compute shaders)
 *
 * Strategy:
 * 1. Upload integer data to temporary R16UI/R16I texture
 * 2. Use shader program that samples from R16UI and writes to R32F
 * 3. Render fullscreen quad to perform conversion
 */
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLTextureFramebufferNormalization
  : public vtkOpenGLTextureNormalizationHelper
{
public:
  vtkOpenGLTextureFramebufferNormalization(vtkOpenGLRenderWindow* context);
  ~vtkOpenGLTextureFramebufferNormalization() override;

  bool ConvertUShortToFloat(const void* sourceData, size_t numValues, int numComps,
    GLuint targetTexture, unsigned int width, unsigned int height) override;

  bool ConvertShortToFloat(const void* sourceData, size_t numValues, int numComps,
    GLuint targetTexture, unsigned int width, unsigned int height) override;

private:
  vtkOpenGLRenderWindow* Context = nullptr;
  GLuint ConversionProgram = 0;
  GLuint IntermediateTexture = 0;
  GLuint ConversionFramebuffer = 0;
  GLuint ConversionVAO = 0;

  bool InitializeConversionShader();
  bool UploadToIntermediateTexture(const void* sourceData, size_t numValues, int numComps,
    unsigned int width, unsigned int height, GLenum internalFormat, GLenum dataFormat);
  bool PerformFramebufferConversion(
    GLuint targetTexture, int numComps, unsigned int width, unsigned int height, float scaleFactor);
};

#endif // vtkOpenGLTextureNormalizationHelper_h
