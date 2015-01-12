/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShadowMapPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkShadowMapBakerPass - Implement a builder of shadow map pass.
// .SECTION Description
// Bake a list of shadow maps, once per spot light.
// It work in conjunction with the vtkShadowMapPass, which uses the
// shadow maps for rendering the opaque geometry (a technique to render hard
// shadows in hardware).
//
// This pass expects an initialized depth buffer and color buffer.
// Initialized buffers means they have been cleared with farest z-value and
// background color/gradient/transparent color.
// An opaque pass may have been performed right after the initialization.
//
//
//
// Its delegate is usually set to a vtkOpaquePass.
//
// .SECTION Implementation
// The first pass of the algorithm is to generate a shadow map per light
// (depth map from the light point of view) by rendering the opaque objects
// with the vtkShadowMapBakerPass::OCCLUDER property keys.
// The second pass is to render the opaque objects with the vtkShadowMap::RECEIVER keys.
//
// .SECTION See Also
// vtkRenderPass, vtkOpaquePass, vtkShadowMapPass

#ifndef vtkShadowMapBakerPass_h
#define vtkShadowMapBakerPass_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRenderPass.h"

class vtkOpenGLRenderWindow;
class vtkInformationIntegerKey;
class vtkCamera;
class vtkLight;
class vtkFrameBufferObject;
class vtkShadowMapBakerPassTextures; // internal
class vtkShadowMapBakerPassLightCameras; // internal

class VTKRENDERINGOPENGL_EXPORT vtkShadowMapBakerPass : public vtkRenderPass
{
public:
  static vtkShadowMapBakerPass *New();
  vtkTypeMacro(vtkShadowMapBakerPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If this key exists on the PropertyKeys of a prop, the prop is viewed as a
  // light occluder (ie it casts shadows). This key is not mutually exclusive
  // with the RECEIVER() key.
  static vtkInformationIntegerKey *OCCLUDER();

  // If this key exists on the Propertykeys of a prop, the prop is viewed as a
  // light/shadow receiver. This key is not mutually exclusive with the
  // OCCLUDER() key.
  static vtkInformationIntegerKey *RECEIVER();

  //BTX
  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
  //ETX

  // Description:
  // Release graphics resources and ask components to release their own
  // resources.
  // \pre w_exists: w!=0
  void ReleaseGraphicsResources(vtkWindow *w);

  // Description:
  // Delegate for rendering the opaque polygonal geometry.
  // If it is NULL, nothing will be rendered and a warning will be emitted.
  // It is usually set to a vtkCameraPass with a sequence of
  // vtkLightPass/vtkOpaquePass.
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(OpaquePass,vtkRenderPass);
  virtual void SetOpaquePass(vtkRenderPass *opaquePass);

  // Description:
  // Delegate for compositing of the shadow maps across processors.
  // If it is NULL, there is no z compositing.
  // It is usually set to a vtkCompositeZPass (Parallel package).
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(CompositeZPass,vtkRenderPass);
  virtual void SetCompositeZPass(vtkRenderPass *compositeZPass);

  // Description:
  // Set/Get the number of pixels in each dimension of the shadow maps
  // (shadow maps are square). Initial value is 256. The greater the better.
  // Resolution does not have to be a power-of-two value.
  vtkSetMacro(Resolution,unsigned int);
  vtkGetMacro(Resolution,unsigned int);

  // Description:
  // Factor used to scale the maximum depth slope of a polygon (definition
  // from OpenGL 2.1 spec section 3.5.5 "Depth Offset" page 112). This is
  // used during the creation the shadow maps (not during mapping of the
  // shadow maps onto the geometry)
  // Play with this value and PolygonOffsetUnits to solve self-shadowing.
  // Valid values can be either positive or negative.
  // Initial value is 1.1f (recommended by the nVidia presentation about
  // Shadow Mapping by Cass Everitt). 3.1f works well with the regression test.
  vtkSetMacro(PolygonOffsetFactor,float);
  vtkGetMacro(PolygonOffsetFactor,float);

  // Description:
  // Factor used to scale an implementation dependent constant that relates
  // to the usable resolution of the depth buffer (definition from OpenGL 2.1
  // spec section 3.5.5 "Depth Offset" page 112). This is
  // used during the creation the shadow maps (not during mapping of the
  // shadow maps onto the geometry)
  // Play with this value and PolygonOffsetFactor to solve self-shadowing.
  // Valid values can be either positive or negative.
  // Initial value is 4.0f (recommended by the nVidia presentation about
  // Shadow Mapping by Cass Everitt). 10.0f works well with the regression
  // test.
  vtkSetMacro(PolygonOffsetUnits,float);
  vtkGetMacro(PolygonOffsetUnits,float);

  // Description:
  // INTERNAL USE ONLY.
  // Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.
  //
  // Tell if there is at least one shadow.
  // Initial value is false.
  bool GetHasShadows();

  // Description:
  // INTERNAL USE ONLY.
  // Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.
  //
  // Tell if the light `l' can create shadows.
  // The light has to not be a head light and to be directional or
  // positional with an angle less than 180 degrees.
  // \pre l_exists: l!=0
  bool LightCreatesShadow(vtkLight *l);

//BTX
  // Description:
  // INTERNAL USE ONLY
  // Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.
  //
  // Give access to the baked shadow maps.
  vtkShadowMapBakerPassTextures *GetShadowMaps();

  // Description:
  // INTERNAL USE ONLY.
  // Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.
  //
  // Give access the cameras builds from the ligths.
  vtkShadowMapBakerPassLightCameras *GetLightCameras();
//ETX

  // Description:
  // INTERNAL USE ONLY.
  // Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.
  //
  // Do the shadows need to be updated?
  // Value changed by vtkShadowMapBakerPass and used by vtkShadowMapPass.
  // Initial value is true.
  bool GetNeedUpdate();

  // // Description:
  // INTERNAL USE ONLY.
  // Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.
  //
  // Set NeedUpate to false. Called by vtkShadowMapPass.
  void SetUpToDate();

 protected:
  // Description:
  // Default constructor. DelegatetPass is set to NULL.
  vtkShadowMapBakerPass();

  // Description:
  // Destructor.
  virtual ~vtkShadowMapBakerPass();

  // Description:
  // Helper method to compute the mNearest point in a given direction.
  // To be called several times, with initialized = false the first time.
  // v: point
  // pt: origin of the direction
  // dir: direction
  void PointNearFar(double *v,
                    double *pt,
                    double *dir,
                    double &mNear,
                    double &mFar,
                    bool initialized);

  // Description:
  // Compute the min/max of the projection of a box in a given direction.
  // bb: bounding box
  // pt: origin of the direction
  // dir: direction
  void BoxNearFar(double *bb,
                  double *pt,
                  double *dir,
                  double &mNear,
                  double &mFar);

  //BTX
  // Description:
  // Build a camera from spot light parameters.
  // \pre light_exists: light!=0
  // \pre lcamera_exists: lcamera!=0
  void BuildCameraLight(vtkLight *light,
                        double *boundingBox,
                        vtkCamera *lcamera);
  //ETX

  // Description:
  // Check if shadow mapping is supported by the current OpenGL context.
  // \pre w_exists: w!=0
  void CheckSupport(vtkOpenGLRenderWindow *w);

  vtkRenderPass *OpaquePass;

  vtkRenderPass *CompositeZPass;

  unsigned int Resolution;

  float PolygonOffsetFactor;
  float PolygonOffsetUnits;

  bool HasShadows;

  // Description:
  // Graphics resources.
  vtkFrameBufferObject *FrameBufferObject;

  vtkShadowMapBakerPassTextures *ShadowMaps;
  vtkShadowMapBakerPassLightCameras *LightCameras;


  vtkTimeStamp LastRenderTime;
  bool NeedUpdate;

private:
  vtkShadowMapBakerPass(const vtkShadowMapBakerPass&);  // Not implemented.
  void operator=(const vtkShadowMapBakerPass&);  // Not implemented.
};

#endif
