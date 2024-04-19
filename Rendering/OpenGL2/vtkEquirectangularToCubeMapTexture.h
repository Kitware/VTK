// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEquirectangularToCubeMapTexture
 * @brief   compute a cubemap texture based on a standard equirectangular projection
 *
 * This special texture converts a 2D projected texture in equirectangular format to a 3D cubemap
 * using the GPU.
 * The generated texture can be used as input for a skybox or an environment map for PBR shading.
 *
 * @sa vtkSkybox vtkRenderer::SetEnvironmentCubeMap
 */

#ifndef vtkEquirectangularToCubeMapTexture_h
#define vtkEquirectangularToCubeMapTexture_h

#include "vtkOpenGLTexture.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLTexture;
class vtkRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkEquirectangularToCubeMapTexture : public vtkOpenGLTexture
{
public:
  static vtkEquirectangularToCubeMapTexture* New();
  vtkTypeMacro(vtkEquirectangularToCubeMapTexture, vtkOpenGLTexture);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the input equirectangular 2D texture.
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
   * Set/Get size of each face of the output cubemap texture.
   * Default is 512.
   */
  vtkGetMacro(CubeMapSize, unsigned int);
  vtkSetMacro(CubeMapSize, unsigned int);
  ///@}

  /**
   * Release any graphics resources that are being consumed by this texture.
   * The parameter window could be used to determine which graphic
   * resources to release. Using the same texture object in multiple
   * render windows is NOT currently supported.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkEquirectangularToCubeMapTexture();
  ~vtkEquirectangularToCubeMapTexture() override;

  unsigned int CubeMapSize = 512;
  vtkOpenGLTexture* InputTexture = nullptr;

private:
  vtkEquirectangularToCubeMapTexture(const vtkEquirectangularToCubeMapTexture&) = delete;
  void operator=(const vtkEquirectangularToCubeMapTexture&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
