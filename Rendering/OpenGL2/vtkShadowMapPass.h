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

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderPass.h"
#include <vector>  // STL Header
#include <string> // For member variables.

class vtkOpenGLRenderWindow;
class vtkInformationIntegerKey;
class vtkCamera;
class vtkLight;
class vtkFrameBufferObject;
class vtkShadowMapPassTextures; // internal
class vtkShadowMapPassLightCameras; // internal
class vtkImageExport;
class vtkTextureObject;
class vtkImplicitHalo;
class vtkSampleFunction;
class vtkShadowMapBakerPass;
class vtkInformationObjectBaseKey;
class vtkShaderProgram;

class VTKRENDERINGOPENGL2_EXPORT vtkShadowMapPass : public vtkRenderPass
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
  // Pass that render the lights and opaque geometry
  // Typically a sequence pass with a light pass and opaque pass.
  vtkGetObjectMacro(OpaqueSequence,vtkRenderPass);
  virtual void SetOpaqueSequence(vtkRenderPass *opaqueSequence);

  // Description:
  // get the matricies for all the
  // shadow maps.
  std::vector<double> ShadowMapTransforms() {
    return this->ShadowTransforms; }

  // Description:
  // get the texture units for the shadow maps
  // for each light. If a light does not cast a shadow
  // it is set to -1
  std::vector<int> GetShadowMapTextureUnits() {
    return this->ShadowTextureUnits; }

  // Description:
  // this key will contain the shadow map pass
  static vtkInformationObjectBaseKey *ShadowMapPass();

  // Description:
  // Get the shader code to compute light factors based
  // on a mappers vertexVC variable
  std::string GetFragmentDeclaration() {
    return this->FragmentDeclaration; }
  std::string GetFragmentImplementation() {
    return this->FragmentImplementation; }

  // Description:
  // A mapper can call this to set the uniforms that this
  // pass uses
  void SetUniforms(vtkShaderProgram *program);

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

  vtkRenderPass *OpaqueSequence;

  // Description:
  // Graphics resources.
  vtkFrameBufferObject *FrameBufferObject;

  vtkShadowMapPassTextures *ShadowMaps;
  vtkShadowMapPassLightCameras *LightCameras;

  vtkTextureObject *IntensityMap;

  vtkSampleFunction *IntensitySource;
  vtkImageExport *IntensityExporter;
  vtkImplicitHalo *Halo;

  vtkTimeStamp LastRenderTime;

  // to store the shader code and settings
  void BuildShaderCode();
  std::string FragmentDeclaration;
  std::string FragmentImplementation;
  std::vector<int> ShadowTextureUnits;
  std::vector<double> ShadowTransforms;
  std::vector<float> ShadowAttenuation;

private:
  vtkShadowMapPass(const vtkShadowMapPass&);  // Not implemented.
  void operator=(const vtkShadowMapPass&);  // Not implemented.
};

#endif
