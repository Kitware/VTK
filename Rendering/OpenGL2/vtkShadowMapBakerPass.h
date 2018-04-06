/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShadowMapBakerPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkShadowMapBakerPass
 * @brief   Implement a builder of shadow map pass.
 *
 * Bake a list of shadow maps, once per spot light.
 * It work in conjunction with the vtkShadowMapPass, which uses the
 * shadow maps for rendering the opaque geometry (a technique to render hard
 * shadows in hardware).
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 * An opaque pass may have been performed right after the initialization.
 *
 *
 *
 * Its delegate is usually set to a vtkOpaquePass.
 *
 * @par Implementation:
 * The first pass of the algorithm is to generate a shadow map per light
 * (depth map from the light point of view) by rendering the opaque objects
 *
 * @sa
 * vtkRenderPass, vtkOpaquePass, vtkShadowMapPass
*/

#ifndef vtkShadowMapBakerPass_h
#define vtkShadowMapBakerPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSmartPointer.h" // for ivars
#include <vector> // STL Header
#include "vtkOpenGLRenderPass.h"

class vtkOpenGLRenderWindow;
class vtkInformationIntegerKey;
class vtkCamera;
class vtkLight;
class vtkOpenGLFramebufferObject;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT vtkShadowMapBakerPass : public vtkOpenGLRenderPass
{
public:
  static vtkShadowMapBakerPass *New();
  vtkTypeMacro(vtkShadowMapBakerPass,vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const vtkRenderState *s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(vtkWindow *w) override;

  //@{
  /**
   * Delegate for rendering the camera, lights, and opaque geometry.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It defaults to a vtkCameraPass with a sequence of
   * vtkLightPass/vtkOpaquePass.
   */
  vtkGetObjectMacro(OpaqueSequence,vtkRenderPass);
  virtual void SetOpaqueSequence(vtkRenderPass *opaqueSequence);
  //@}

  //@{
  /**
   * Delegate for compositing of the shadow maps across processors.
   * If it is NULL, there is no z compositing.
   * It is usually set to a vtkCompositeZPass (Parallel package).
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(CompositeZPass,vtkRenderPass);
  virtual void SetCompositeZPass(vtkRenderPass *compositeZPass);
  //@}

  //@{
  /**
   * Set/Get the number of pixels in each dimension of the shadow maps
   * (shadow maps are square). Initial value is 256. The greater the better.
   * Resolution does not have to be a power-of-two value.
   */
  vtkSetMacro(Resolution,unsigned int);
  vtkGetMacro(Resolution,unsigned int);
  //@}

  /**
   * INTERNAL USE ONLY.
   * Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.

   * Tell if there is at least one shadow.
   * Initial value is false.
   */
  bool GetHasShadows();

  /**
   * INTERNAL USE ONLY.
   * Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.

   * Tell if the light `l' can create shadows.
   * The light has to not be a head light and to be directional or
   * positional with an angle less than 180 degrees.
   * \pre l_exists: l!=0
   */
  bool LightCreatesShadow(vtkLight *l);

  /**
   * INTERNAL USE ONLY
   * Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.

   * Give access to the baked shadow maps.
   */
  std::vector<vtkSmartPointer<vtkTextureObject> > *GetShadowMaps();

  /**
   * INTERNAL USE ONLY.
   * Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.

   * Give access the cameras builds from the ligths.
   */
  std::vector<vtkSmartPointer<vtkCamera> > *GetLightCameras();

  /**
   * INTERNAL USE ONLY.
   * Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.

   * Do the shadows need to be updated?
   * Value changed by vtkShadowMapBakerPass and used by vtkShadowMapPass.
   * Initial value is true.
   */
  bool GetNeedUpdate();

  // // Description:
  // INTERNAL USE ONLY.
  // Internally used by vtkShadowMapBakerPass and vtkShadowMapPass.
  //
  // Set NeedUpate to false. Called by vtkShadowMapPass.
  void SetUpToDate();

 protected:
  /**
   * Default constructor. DelegatetPass is set to NULL.
   */
  vtkShadowMapBakerPass();

  /**
   * Destructor.
   */
  ~vtkShadowMapBakerPass() override;

  // vtkOpenGLRenderPass virtuals:
  bool PreReplaceShaderValues(std::string &vertexShader,
                                   std::string &geometryShader,
                                   std::string &fragmentShader,
                                   vtkAbstractMapper *mapper,
                                   vtkProp *prop) override;
  bool SetShaderParameters(vtkShaderProgram *program,
                          vtkAbstractMapper *mapper, vtkProp *prop,
                          vtkOpenGLVertexArrayObject* VAO = nullptr) override;

  /**
   * Helper method to compute the mNearest point in a given direction.
   * To be called several times, with initialized = false the first time.
   * v: point
   * pt: origin of the direction
   * dir: direction
   */
  void PointNearFar(double *v,
                    double *pt,
                    double *dir,
                    double &mNear,
                    double &mFar,
                    bool initialized);

  /**
   * Compute the min/max of the projection of a box in a given direction.
   * bb: bounding box
   * pt: origin of the direction
   * dir: direction
   */
  void BoxNearFar(double *bb,
                  double *pt,
                  double *dir,
                  double &mNear,
                  double &mFar);

  /**
   * Build a camera from spot light parameters.
   * \pre light_exists: light!=0
   * \pre lcamera_exists: lcamera!=0
   */
  void BuildCameraLight(vtkLight *light,
                        double *boundingBox,
                        vtkCamera *lcamera);

  /**
   * Check if shadow mapping is supported by the current OpenGL context.
   * \pre w_exists: w!=0
   */
  void CheckSupport(vtkOpenGLRenderWindow *w);

  vtkRenderPass *OpaqueSequence;

  vtkRenderPass *CompositeZPass;

  unsigned int Resolution;

  bool HasShadows;

  /**
   * Graphics resources.
   */
  vtkOpenGLFramebufferObject *FrameBufferObject;

  std::vector<vtkSmartPointer<vtkTextureObject> > *ShadowMaps;
  std::vector<vtkSmartPointer<vtkCamera> > *LightCameras;


  vtkTimeStamp LastRenderTime;
  bool NeedUpdate;
  size_t CurrentLightIndex;


private:
  vtkShadowMapBakerPass(const vtkShadowMapBakerPass&) = delete;
  void operator=(const vtkShadowMapBakerPass&) = delete;
};

#endif
