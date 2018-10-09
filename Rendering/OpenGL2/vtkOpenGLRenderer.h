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
#include "vtkSmartPointer.h" // For vtkSmartPointer
#include <vector>  // STL Header
#include <string> // Ivars

class vtkOpenGLFXAAFilter;
class vtkRenderPass;
class vtkOpenGLState;
class vtkOpenGLTexture;
class vtkOrderIndependentTranslucentPass;
class vtkTextureObject;
class vtkDepthPeelingPass;
class vtkShaderProgram;
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
   * Indicate if this system is subject to the Apple/AMD bug
   * of not having a working glPrimitiveId <rdar://20747550>.
   * The bug is fixed on macOS 10.11 and later, and this method
   * will return false when the OS is new enough.
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

  // Get the state object used to keep track of
  // OpenGL state
  vtkOpenGLState *GetState();

  // get the standard lighting uniform declarations
  // for the current set of lights
  const char *GetLightingUniforms();

  // update the lighting uniforms for this shader if they
  // are out of date
  void UpdateLightingUniforms(vtkShaderProgram *prog);

  // get the complexity of the current lights as a int
  // 0 = no lighting
  // 1 = headlight
  // 2 = directional lights
  // 3 = positional lights
  enum LightingComplexityEnum {
    NoLighting = 0,
    Headlight = 1,
    Directional = 2,
    Positional = 3
  };
  vtkGetMacro(LightingComplexity, int);

  // get the number of lights turned on
  vtkGetMacro(LightingCount, int);

  /**
   * Set the user light transform applied after the camera transform.
   * Can be null to disable it.
   */
  void SetUserLightTransform(vtkTransform* transform);

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

  /**
   * Check and return the textured background for the current state
   * If monocular or stereo left eye, check BackgroundTexture
   * If stereo right eye, check RightBackgroundTexture
   */
  vtkTexture* GetCurrentTexturedBackground();

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
   * Fallback for transparency
   */
  vtkOrderIndependentTranslucentPass *TranslucentPass;

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

  std::string LightingDeclaration;
  int LightingComplexity;
  int LightingCount;
  vtkMTimeType LightingUpdateTime;

  /**
   * Optional user transform for lights
   */
  vtkSmartPointer<vtkTransform> UserLightTransform;

private:
  vtkOpenGLRenderer(const vtkOpenGLRenderer&) = delete;
  void operator=(const vtkOpenGLRenderer&) = delete;
};

#endif
