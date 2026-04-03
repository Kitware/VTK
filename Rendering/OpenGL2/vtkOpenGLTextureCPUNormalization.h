// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkOpenGLTextureCPUNormalization
 * @brief CPU-based fallback for 16-bit to float texture normalization
 *
 * Converts 16-bit integer texture data to normalized 32-bit float format
 * on the CPU. This is the fallback implementation used when GPU-assisted
 * conversion is unavailable.
 *
 * @sa vtkOpenGLTextureNormalizationHelper, vtkOpenGLTextureComputeShaderNormalization,
 *     vtkOpenGLTextureFramebufferNormalization
 */

#ifndef vtkOpenGLTextureCPUNormalization_h
#define vtkOpenGLTextureCPUNormalization_h

#include "vtkOpenGLTextureNormalizationHelper.h"
#include "vtkRenderingOpenGL2Module.h"
#include "vtk_glad.h"
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLTextureCPUNormalization
  : public vtkOpenGLTextureNormalizationHelper
{
public:
  static vtkOpenGLTextureCPUNormalization* New();
  vtkTypeMacro(vtkOpenGLTextureCPUNormalization, vtkOpenGLTextureNormalizationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool ConvertUShortToFloat(const void* sourceData, size_t numValues, int numComps,
    unsigned int targetTexture, unsigned int width, unsigned int height) override;

  bool ConvertShortToFloat(const void* sourceData, size_t numValues, int numComps,
    unsigned int targetTexture, unsigned int width, unsigned int height) override;

protected:
  vtkOpenGLTextureCPUNormalization();
  ~vtkOpenGLTextureCPUNormalization() override = default;

  // Reusable buffer to avoid repeated allocations
  std::vector<float> ConversionBuffer;

private:
  vtkOpenGLTextureCPUNormalization(const vtkOpenGLTextureCPUNormalization&) = delete;
  void operator=(const vtkOpenGLTextureCPUNormalization&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLTextureCPUNormalization_h
