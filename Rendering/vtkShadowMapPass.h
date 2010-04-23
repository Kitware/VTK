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
// .NAME vtkShadowMapPass - Implement a shadow mapping render pass.
// .SECTION Description
// Render the opaque polygonal geometry of a scene with shadow maps (a
// technique to render hard shadows in hardware).
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
// with the OCCLUDER property keys.
// The second pass is to render the opaque objects with the RECEIVER keys.
//
// .SECTION See Also
// vtkRenderPass, vtkOpaquePass

#ifndef __vtkShadowMapPass_h
#define __vtkShadowMapPass_h

#include "vtkRenderPass.h"

class vtkOpenGLRenderWindow;
class vtkInformationIntegerKey;
class vtkCamera;
class vtkLight;
class vtkFrameBufferObject;
class vtkShadowMapPassTextures; // internal
class vtkShadowMapPassLightCameras; // internal
class vtkShaderProgram2;
class vtkImageGaussianSource;
class vtkImageExport;
class vtkTextureObject;
class vtkImplicitHalo;
class vtkSampleFunction;

class VTK_RENDERING_EXPORT vtkShadowMapPass : public vtkRenderPass
{
public:
  static vtkShadowMapPass *New();
  vtkTypeMacro(vtkShadowMapPass,vtkRenderPass);
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
  // It is usually set to a vtkTranslucentPass.
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(OpaquePass,vtkRenderPass);
  virtual void SetOpaquePass(vtkRenderPass *opaquePass);

  // Description:
  // Delegate for rendering the opaque polygonal geometry.
  // If it is NULL, nothing will be rendered and a warning will be emitted.
  // It is usually set to a vtkTranslucentPass.
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(CompositeZPass,vtkRenderPass);
  virtual void SetCompositeZPass(vtkRenderPass *opaquePass);

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

 protected:
  // Description:
  // Default constructor. DelegatetPass is set to NULL.
  vtkShadowMapPass();

  // Description:
  // Destructor.
  virtual ~vtkShadowMapPass();

  //BTX
  // Description:
  // Build a camera from spot light parameters.
  // \pre light_exists: light!=0
  // \pre light_is_spotlight: light->GetConeAngle()<180.0
  // \pre camera_exists: camera!=0
  void BuildCameraLight(vtkLight *light, double *boundingBox, vtkCamera *lcamera);
  //ETX

  // Description:
  // Build the intensity map.
  void BuildSpotLightIntensityMap();

  // Description:
  // Check if shadow mapping is supported by the current OpenGL context.
  // \pre w_exists: w!=0
  void CheckSupport(vtkOpenGLRenderWindow *w);

  vtkRenderPass *OpaquePass;
  vtkRenderPass *CompositeZPass;
  unsigned int Resolution;

  float PolygonOffsetFactor;
  float PolygonOffsetUnits;

  // Description:
  // Graphics resources.
  vtkFrameBufferObject *FrameBufferObject;

  vtkShadowMapPassTextures *ShadowMaps;
  vtkShadowMapPassLightCameras *LightCameras;
  vtkShaderProgram2 *Program;

  vtkTextureObject *IntensityMap;

//  vtkImageGaussianSource *IntensitySource;
  vtkSampleFunction *IntensitySource;
  vtkImageExport *IntensityExporter;
  vtkImplicitHalo *Halo;

  vtkTimeStamp LastRenderTime;

private:
  vtkShadowMapPass(const vtkShadowMapPass&);  // Not implemented.
  void operator=(const vtkShadowMapPass&);  // Not implemented.
};

#endif
