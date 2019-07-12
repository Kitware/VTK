/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLFluidMapper
 * @brief   draw a fluid using spheres
 *
 * An OpenGL mapper that uses sphere imposters and SS passes to render as
 * fluid represented as points
 */

#ifndef vtkOpenGLFluidMapper_h
#define vtkOpenGLFluidMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkAbstractVolumeMapper.h"
#include <map> //for methods
#include "vtkShader.h" // for methods
#include "vtkOpenGLHelper.h" // used for ivars

class vtkMatrix4x4;
class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkOpenGLState;
class vtkOpenGLQuadHelper;
class vtkOpenGLVertexBufferObjectGroup;
class vtkPolyData;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLFluidMapper : public vtkAbstractVolumeMapper
{
public:
  static vtkOpenGLFluidMapper* New();
  vtkTypeMacro(vtkOpenGLFluidMapper, vtkAbstractVolumeMapper)
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(vtkPolyData *in);
  //@}

  //@{
  /**
   * This value will be used for the radius
   */
   vtkSetMacro(Radius, float);
   vtkGetMacro(Radius, float);

  /**
   * This calls RenderPiece
   */
  void Render(vtkRenderer *ren, vtkVolume *vol) override;

protected:
  vtkOpenGLFluidMapper();
  ~vtkOpenGLFluidMapper() override;

  /**
   * Perform string replacements on the shader templates
   */
  virtual void UpdateShaders(
    vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkVolume *vol);

  /**
   * Set the shader parameters related to the Camera
   */
  virtual void SetCameraShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkVolume *vol);

  /**
   * Set the shader parameters related to the actor/mapper
   */
  virtual void SetMapperShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkVolume *vol);

  virtual void RenderVolume(vtkRenderer *ren, vtkVolume *vol);

  float Radius;

  //@{
  /**
   * Cache viewport values
   */
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;
  //@}

  vtkOpenGLFramebufferObject *Framebuffer;

  vtkOpenGLQuadHelper *FinalBlend;
  vtkOpenGLQuadHelper *IntermediateBlend;

  // The VBO and its layout.
  vtkOpenGLVertexBufferObjectGroup *VBOs;
  vtkTimeStamp VBOBuildTime; // When was the OpenGL VBO updated?
  vtkOpenGLHelper CellBO;

  enum
  {
    OpaqueZ = 0,
    OpaqueRGBA,
    FluidZ,
    FluidThickness
  };
  vtkTextureObject *Textures[4];

  vtkMatrix4x4* TempMatrix4;

private:
  vtkOpenGLFluidMapper(const vtkOpenGLFluidMapper&) = delete;
  void operator=(const vtkOpenGLFluidMapper&) = delete;
};

#endif
