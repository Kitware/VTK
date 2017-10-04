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

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderer.h"
#include <vector>  // STL Header

class vtkOpenGLFXAAFilter;
class vtkRenderPass;
class vtkOpenGLTexture;
class vtkTextureObject;
class vtkDepthPeelingPass;
class vtkShadowMapPass;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLRenderer : public vtkRenderer
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

  /**
   * Indicate if this system is subject to the apple/amd bug
   * of not having a working glPrimitiveId
   */
  bool HaveApplePrimitiveIdBug();

  /**
   * Indicate if this system is subject to the apple/NVIDIA bug that causes
   * crashes in the driver when too many query objects are allocated.
   */
  static bool HaveAppleQueryAllocationBug();

  /**
   * Dual depth peeling may be disabled for certain runtime configurations.
   * This method returns true if vtkDualDepthPeelingPass will be used in place
   * of vtkDepthPeelingPass.
   */
  bool IsDualDepthPeelingSupported();

protected:
  vtkOpenGLRenderer();
  ~vtkOpenGLRenderer() override;

  /**
   * Check the compilation status of some fragment shader source.
   */
  void CheckCompilation(unsigned int fragmentShader);

  // Internal method to release graphics resources in any derived renderers.
  void ReleaseGraphicsResources(vtkWindow *w) override;

  /**
   * Ask all props to update and draw any opaque and translucent
   * geometry. This includes both vtkActors and vtkVolumes
   * Returns the number of props that rendered geometry.
   */
  int UpdateGeometry() override;

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

  friend class vtkOpenGLProperty;
  friend class vtkOpenGLTexture;
  friend class vtkOpenGLImageSliceMapper;
  friend class vtkOpenGLImageResliceMapper;

  /**
   * FXAA is delegated to an instance of vtkOpenGLFXAAFilter
   */
  vtkOpenGLFXAAFilter *FXAAFilter;

  /**
   * Depth peeling is delegated to an instance of vtkDepthPeelingPass
   */
  vtkDepthPeelingPass *DepthPeelingPass;

  /**
   * Shadows are delegated to an instance of vtkShadowMapPass
   */
  vtkShadowMapPass *ShadowMapPass;

  // Is rendering at translucent geometry stage using depth peeling and
  // rendering a layer other than the first one? (Boolean value)
  // If so, the uniform variables UseTexture and Texture can be set.
  // (Used by vtkOpenGLProperty or vtkOpenGLTexture)
  int DepthPeelingHigherLayer;

  friend class vtkRenderPass;

  bool HaveApplePrimitiveIdBugValue;
  bool HaveApplePrimitiveIdBugChecked;

private:
  vtkOpenGLRenderer(const vtkOpenGLRenderer&) = delete;
  void operator=(const vtkOpenGLRenderer&) = delete;
};

#endif
