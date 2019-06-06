/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBRIrradianceTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPBRIrradianceTexture
 * @brief   precompute irradiance texture used in physically based rendering
 *
 * Irradiance texture is a cubemap which average light of a hemisphere of the input cubemap.
 * It is used in Image Base Lighting to compute the diffuse part.
 */

#ifndef vtkPBRIrradianceTexture_h
#define vtkPBRIrradianceTexture_h

#include "vtkOpenGLTexture.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLTexture;
class vtkRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkPBRIrradianceTexture : public vtkOpenGLTexture
{
public:
  static vtkPBRIrradianceTexture* New();
  vtkTypeMacro(vtkPBRIrradianceTexture, vtkOpenGLTexture);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the input cubemap.
   */
  void SetInputCubeMap(vtkOpenGLTexture* texture);
  vtkGetObjectMacro(InputCubeMap, vtkOpenGLTexture);
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
   * Set/Get size of texture.
   * Default is 512.
   */
  vtkGetMacro(IrradianceSize, unsigned int);
  vtkSetMacro(IrradianceSize, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the size of steps in radians used to sample hemisphere.
   * Default is (pi/64).
   * In some OpenGL drivers (OSMesa, old OSX), the default value might be too high leading to
   * artifacts.
   */
  vtkGetMacro(IrradianceStep, float);
  vtkSetMacro(IrradianceStep, float);
  //@}

protected:
  vtkPBRIrradianceTexture() = default;
  ~vtkPBRIrradianceTexture() override;

  float IrradianceStep = 0.04908738521; // pi / 64
  unsigned int IrradianceSize = 512;
  vtkOpenGLTexture* InputCubeMap = nullptr;

private:
  vtkPBRIrradianceTexture(const vtkPBRIrradianceTexture&) = delete;
  void operator=(const vtkPBRIrradianceTexture&) = delete;
};

#endif
