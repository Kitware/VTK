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

#ifndef vtkShadowMapPass_h
#define vtkShadowMapPass_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRenderPass.h"

class vtkOpenGLRenderWindow;
class vtkInformationIntegerKey;
class vtkCamera;
class vtkLight;
class vtkFrameBufferObject;
class vtkShadowMapPassTextures; // internal
class vtkShadowMapPassLightCameras; // internal
class vtkShaderProgram2;
class vtkImageExport;
class vtkTextureObject;
class vtkImplicitHalo;
class vtkSampleFunction;
class vtkShadowMapBakerPass;

class VTKRENDERINGOPENGL_EXPORT vtkShadowMapPass : public vtkRenderPass
{
public:
  static vtkShadowMapPass *New();
  vtkTypeMacro(vtkShadowMapPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // Pass that generates the shadow maps.
  // the vtkShadowMapPass will use the Resolution ivar of
  // this pass.
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(ShadowMapBakerPass,vtkShadowMapBakerPass);
  virtual void SetShadowMapBakerPass(
    vtkShadowMapBakerPass *shadowMapBakerPass);

  // Description:
  // Pass that render the opaque geometry, with no camera pass (otherwise
  // it does not work with Ice-T).
  // Initial value is a NULL pointer.
  // Typically a sequence pass with a light pass and opaque pass.
  // This should be the Opaque pass of the vtkShadowMapBakerPass without the
  // vtkCameraPass.
  vtkGetObjectMacro(OpaquePass,vtkRenderPass);
  virtual void SetOpaquePass(vtkRenderPass *opaquePass);

 protected:
  // Description:
  // Default constructor. DelegatetPass is set to NULL.
  vtkShadowMapPass();

  // Description:
  // Destructor.
  virtual ~vtkShadowMapPass();

  // Description:
  // Build the intensity map.
  void BuildSpotLightIntensityMap();

  // Description:
  // Check if shadow mapping is supported by the current OpenGL context.
  // \pre w_exists: w!=0
  void CheckSupport(vtkOpenGLRenderWindow *w);

  vtkShadowMapBakerPass *ShadowMapBakerPass;
  vtkRenderPass *CompositeRGBAPass;

  vtkRenderPass *OpaquePass;

  // Description:
  // Graphics resources.
  vtkFrameBufferObject *FrameBufferObject;

  vtkShadowMapPassTextures *ShadowMaps;
  vtkShadowMapPassLightCameras *LightCameras;
  vtkShaderProgram2 *Program;

  vtkTextureObject *IntensityMap;

  vtkSampleFunction *IntensitySource;
  vtkImageExport *IntensityExporter;
  vtkImplicitHalo *Halo;

  vtkTimeStamp LastRenderTime;

private:
  vtkShadowMapPass(const vtkShadowMapPass&);  // Not implemented.
  void operator=(const vtkShadowMapPass&);  // Not implemented.
};

#endif
