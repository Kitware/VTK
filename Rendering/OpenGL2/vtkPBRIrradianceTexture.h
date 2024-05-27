// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPBRIrradianceTexture
 * @brief   precompute irradiance texture used in physically based rendering
 *
 * Irradiance texture is a cubemap which average light of a hemisphere of the input texture.
 * The input texture can be a cubemap or an equirectangular projection.
 * It is used in Image Base Lighting to compute the diffuse part.
 */

#ifndef vtkPBRIrradianceTexture_h
#define vtkPBRIrradianceTexture_h

#include "vtkOpenGLTexture.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLTexture;
class vtkRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkPBRIrradianceTexture : public vtkOpenGLTexture
{
public:
  static vtkPBRIrradianceTexture* New();
  vtkTypeMacro(vtkPBRIrradianceTexture, vtkOpenGLTexture);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the input texture.
   */
  void SetInputTexture(vtkOpenGLTexture* texture);
  vtkGetObjectMacro(InputTexture, vtkOpenGLTexture);
  ///@}

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
   * Default is 256.
   */
  vtkGetMacro(IrradianceSize, unsigned int);
  vtkSetMacro(IrradianceSize, unsigned int);
  ///@}

  ///@{
  /**
   * Set/Get the size of steps in radians used to sample hemisphere.
   * Default is (pi/64).
   * In some OpenGL drivers (OSMesa, old OSX), the default value might be too low leading to
   * artifacts.
   */
  vtkGetMacro(IrradianceStep, float);
  vtkSetMacro(IrradianceStep, float);
  ///@}

  ///@{
  /**
   * Set/Get the conversion to linear color space.
   * If the input texture is in sRGB color space and the conversion is not done by OpenGL
   * directly with the texture format, the conversion can be done in the shader with this flag.
   */
  vtkGetMacro(ConvertToLinear, bool);
  vtkSetMacro(ConvertToLinear, bool);
  vtkBooleanMacro(ConvertToLinear, bool);
  ///@}

  /**
   * Release any graphics resources that are being consumed by this texture.
   * The parameter window could be used to determine which graphic
   * resources to release. Using the same texture object in multiple
   * render windows is NOT currently supported.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkPBRIrradianceTexture() = default;
  ~vtkPBRIrradianceTexture() override;

  float IrradianceStep = 0.04908738521; // pi / 64
  unsigned int IrradianceSize = 256;
  vtkOpenGLTexture* InputTexture = nullptr;
  bool ConvertToLinear = false;

private:
  vtkPBRIrradianceTexture(const vtkPBRIrradianceTexture&) = delete;
  void operator=(const vtkPBRIrradianceTexture&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
