/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEquirectangularToCubemapTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkEquirectangularToCubemapTexture
 * @brief   compute a cubemap texture based on an standard equirectangular projection
 *
 * This special texture converts a 2D projected texture in equirectangular format to a 3D cubemap
 * using the GPU.
 */

#ifndef vtkEquirectangularToCubemapTexture_h
#define vtkEquirectangularToCubemapTexture_h

#include "vtkOpenGLTexture.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLTexture;
class vtkRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkEquirectangularToCubemapTexture : public vtkOpenGLTexture
{
public:
  static vtkEquirectangularToCubemapTexture* New();
  vtkTypeMacro(vtkEquirectangularToCubemapTexture, vtkOpenGLTexture);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the input equirectangular 2D texture.
   */
  void SetInputTexture(vtkOpenGLTexture* texture);
  vtkGetObjectMacro(InputTexture, vtkOpenGLTexture);
  //@}

  /**
   * Implement base class method.
   */
  void Load(vtkRenderer*) override;

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer* ren) override { this->Load(ren); }

  //@{
  /**
   * Set/Get size of each face of the output cubemap texture.
   * Default is 512.
   */
  vtkGetMacro(CubemapSize, unsigned int);
  vtkSetMacro(CubemapSize, unsigned int);
  //@}

protected:
  vtkEquirectangularToCubemapTexture() = default;
  ~vtkEquirectangularToCubemapTexture() override;

  unsigned int CubemapSize = 512;
  vtkOpenGLTexture* InputTexture = nullptr;

private:
  vtkEquirectangularToCubemapTexture(const vtkEquirectangularToCubemapTexture&) = delete;
  void operator=(const vtkEquirectangularToCubemapTexture&) = delete;
};

#endif
