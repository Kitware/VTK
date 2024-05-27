// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPBRLUTTexture
 * @brief   precompute BRDF look-up table texture used in physically based rendering
 *
 * This texture is a 2D texture which precompute Fresnel response scale (red) and bias (green)
 * based on roughness (x) and angle between light and normal (y).
 */

#ifndef vtkPBRLUTTexture_h
#define vtkPBRLUTTexture_h

#include "vtkOpenGLTexture.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h"           // For vtkSmartPointer
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLTexture;
class vtkRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkPBRLUTTexture : public vtkOpenGLTexture
{
public:
  static vtkPBRLUTTexture* New();
  vtkTypeMacro(vtkPBRLUTTexture, vtkOpenGLTexture);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Load(vtkRenderer*) override;

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer* ren) override { this->Load(ren); }

  ///@{
  /**
   * Set/Get size of texture.
   * Default is 1024.
   */
  vtkGetMacro(LUTSize, unsigned int);
  vtkSetMacro(LUTSize, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get the number of samples used during Monte-Carlo integration.
   * Default is 512.
   */
  vtkGetMacro(LUTSamples, unsigned int);
  vtkSetMacro(LUTSamples, unsigned int);
  ///@}

protected:
  vtkPBRLUTTexture() = default;
  ~vtkPBRLUTTexture() override = default;

  unsigned int LUTSize = 512;
  unsigned int LUTSamples = 1024;

private:
  vtkPBRLUTTexture(const vtkPBRLUTTexture&) = delete;
  void operator=(const vtkPBRLUTTexture&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
