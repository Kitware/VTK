/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLRenderer
 * @brief   OpenGL renderer
 *
 * vtkOpenGLRenderer is a concrete implementation of the abstract class
 * vtkRenderer. vtkOpenGLRenderer interfaces to the OpenGL graphics library.
*/

#ifndef vtkOpenGLRenderer_h
#define vtkOpenGLRenderer_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRenderer.h"

class vtkOpenGLRendererLayerList; // Pimpl
class vtkRenderPass;
class vtkShaderProgram2;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLRenderer : public vtkRenderer
{
public:
  static vtkOpenGLRenderer *New();
  vtkTypeMacro(vtkOpenGLRenderer, vtkRenderer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Concrete open gl render method.
   */
  void DeviceRender(void) override;

  /**
   * Overridden to support hidden line removal.
   */
  void DeviceRenderOpaqueGeometry() override;

  /**
   * Render translucent polygonal geometry. Default implementation just call
   * UpdateTranslucentPolygonalGeometry().
   * Subclasses of vtkRenderer that can deal with depth peeling must
   * override this method.
   */
  void DeviceRenderTranslucentPolygonalGeometry() override;

  /**
   * Internal method temporarily removes lights before reloading them
   * into graphics pipeline.
   */
  void ClearLights(void) override;

  void Clear(void) override;

  /**
   * Ask lights to load themselves into graphics pipeline.
   */
  int UpdateLights(void) override;

  /**
   * Is rendering at translucent geometry stage using depth peeling and
   * rendering a layer other than the first one? (Boolean value)
   * If so, the uniform variables UseTexture and Texture can be set.
   * (Used by vtkOpenGLProperty or vtkOpenGLTexture)
   */
  int GetDepthPeelingHigherLayer();

  //@{
  /**

   */
  vtkGetObjectMacro(ShaderProgram, vtkShaderProgram2);
  virtual void SetShaderProgram(vtkShaderProgram2 *program);
  //@}

protected:
  vtkOpenGLRenderer();
  ~vtkOpenGLRenderer() override;

  /**
   * Check the compilation status of some fragment shader source.
   */
  void CheckCompilation(unsigned int fragmentShader);

  // Internal method to release graphics resources in any derived renderers.
  void ReleaseGraphicsResources(vtkWindow *w) override;

  // Picking functions to be implemented by sub-classes
  void DevicePickRender() override;
  void StartPick(unsigned int pickFromSize) override;
  void UpdatePickId() override;
  void DonePick() override;
  unsigned int GetPickedId() override;
  unsigned int GetNumPickedIds() override;
  int GetPickedIds(unsigned int atMost, unsigned int *callerBuffer) override;
  double GetPickedZ() override;

  // Ivars used in picking
  class vtkGLPickInfo* PickInfo;

  double PickedZ;

  int NumberOfLightsBound;
  /**
   * Render a peel layer. If there is no more GPU RAM to save the texture,
   * return false otherwise returns true. Also if layer==0 and no prop have
   * been rendered (there is no translucent geometry), it returns false.
   * \pre positive_layer: layer>=0
   */
  int RenderPeel(int layer);

  friend class vtkOpenGLProperty;
  friend class vtkOpenGLTexture;
  friend class vtkOpenGLImageSliceMapper;
  friend class vtkOpenGLImageResliceMapper;

  /**
   * Access to the OpenGL program shader uniform variable "useTexture" from the
   * vtkOpenGLProperty or vtkOpenGLTexture.
   */
  int GetUseTextureUniformVariable();

  /**
   * Access to the OpenGL program shader uniform variable "texture" from the
   * vtkOpenGLProperty or vtkOpenGLTexture.
   */
  int GetTextureUniformVariable();

  /**
   * This flag is on if the current OpenGL context supports extensions
   * required by the depth peeling technique.
   */
  int DepthPeelingIsSupported;

  /**
   * This flag is on once the OpenGL extensions required by the depth peeling
   * technique have been checked.
   */
  int DepthPeelingIsSupportedChecked;

  /**
   * Used by the depth peeling technique to store the transparency layers.
   */
  vtkOpenGLRendererLayerList *LayerList;

  unsigned int OpaqueLayerZ;
  unsigned int TransparentLayerZ;
  unsigned int ProgramShader;

  //@{
  /**
   * Cache viewport values for depth peeling.
   */
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;
  //@}

  /**
   * Actual depth format: vtkgl::DEPTH_COMPONENT16_ARB
   * or vtkgl::DEPTH_COMPONENT24_ARB
   */
  unsigned int DepthFormat;

  // Is rendering at translucent geometry stage using depth peeling and
  // rendering a layer other than the first one? (Boolean value)
  // If so, the uniform variables UseTexture and Texture can be set.
  // (Used by vtkOpenGLProperty or vtkOpenGLTexture)
  int DepthPeelingHigherLayer;

  vtkShaderProgram2 *ShaderProgram;

  friend class vtkRenderPass;

private:
  vtkOpenGLRenderer(const vtkOpenGLRenderer&) = delete;
  void operator=(const vtkOpenGLRenderer&) = delete;
};

#endif
