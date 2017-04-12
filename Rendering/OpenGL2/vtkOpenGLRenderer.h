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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Concrete open gl render method.
   */
  void DeviceRender(void) VTK_OVERRIDE;

  /**
   * Overridden to support hidden line removal.
   */
  void DeviceRenderOpaqueGeometry() VTK_OVERRIDE;

  /**
   * Render translucent polygonal geometry. Default implementation just call
   * UpdateTranslucentPolygonalGeometry().
   * Subclasses of vtkRenderer that can deal with depth peeling must
   * override this method.
   */
  void DeviceRenderTranslucentPolygonalGeometry() VTK_OVERRIDE;

  void Clear(void) VTK_OVERRIDE;

  /**
   * Ask lights to load themselves into graphics pipeline.
   */
  int UpdateLights(void) VTK_OVERRIDE;

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
   * Dual depth peeling may be disabled for certain runtime configurations.
   * This method returns true if vtkDualDepthPeelingPass will be used in place
   * of vtkDepthPeelingPass.
   */
  bool IsDualDepthPeelingSupported();

protected:
  vtkOpenGLRenderer();
  ~vtkOpenGLRenderer() VTK_OVERRIDE;

  /**
   * Check the compilation status of some fragment shader source.
   */
  void CheckCompilation(unsigned int fragmentShader);

  // Internal method to release graphics resources in any derived renderers.
  void ReleaseGraphicsResources(vtkWindow *w) VTK_OVERRIDE;

  /**
   * Ask all props to update and draw any opaque and translucent
   * geometry. This includes both vtkActors and vtkVolumes
   * Returns the number of props that rendered geometry.
   */
  int UpdateGeometry() VTK_OVERRIDE;

  // Picking functions to be implemented by sub-classes
  void DevicePickRender() VTK_OVERRIDE;
  void StartPick(unsigned int pickFromSize) VTK_OVERRIDE;
  void UpdatePickId() VTK_OVERRIDE;
  void DonePick() VTK_OVERRIDE;
  unsigned int GetPickedId() VTK_OVERRIDE;
  unsigned int GetNumPickedIds() VTK_OVERRIDE;
  int GetPickedIds(unsigned int atMost, unsigned int *callerBuffer) VTK_OVERRIDE;
  double GetPickedZ() VTK_OVERRIDE;

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
  vtkOpenGLRenderer(const vtkOpenGLRenderer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLRenderer&) VTK_DELETE_FUNCTION;
};

#endif
