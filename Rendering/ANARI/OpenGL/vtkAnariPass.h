// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariPass
 * @brief   a render pass that uses ANARI (ANAlytic Rendering Interface)
 *          instead of OpenGL.
 *
 *
 * ANARI provides cross-vendor portability to diverse rendering engines,
 * including those using state-of-the-art ray tracing. This is a render
 * pass that can be put into a vtkRenderWindow which makes it use the
 * back-end loaded with ANARI instead of OpenGL to render. Adding or
 * removing the pass will swap back and forth between the two.
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 *
 */

#ifndef vtkAnariPass_h
#define vtkAnariPass_h

#include "vtkNew.h" // For vtkNew
#include "vtkRenderPass.h"
#include "vtkRenderingAnariOpenGLModule.h" // For export macro

#include "vtkAnariDevice.h"     // For vtkAnariDevice
#include "vtkAnariRenderer.h"   // For vtkAnariRenderer
#include "vtkAnariSceneGraph.h" // For vtkAnariSceneGraph

VTK_ABI_NAMESPACE_BEGIN

// Forward declarations
class vtkAnariViewNodeFactory;
class vtkCameraPass;
class vtkOpenGLFramebufferObject;
class vtkOpenGLQuadHelper;
class vtkOpenGLRenderWindow;
class vtkTextureObject;
class vtkViewNodeFactory;

class VTKRENDERINGANARIOPENGL_EXPORT vtkAnariPass : public vtkRenderPass
{
public:
  static vtkAnariPass* New();
  vtkTypeMacro(vtkAnariPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const vtkRenderState* s) override;

  //@{
  /**
   * Get the root of the underlying scene graph.
   */
  vtkGetObjectMacro(SceneGraph, vtkAnariSceneGraph);
  //@}

  /**
   * Get the managing class of the ANARI device for queries or make changes.
   */
  vtkAnariDevice* GetAnariDevice();

  /**
   * Get the managing class of the ANARI renderer to query or make changes. Note
   * that this will not do anything unless the device has been initialized in
   * the device .
   */
  vtkAnariRenderer* GetAnariRenderer();

  /**
   * Make the factory available to apps that need to replace object(s) in VTK with
   * their own at runtime (e.g. VisIt).
   */
  virtual vtkViewNodeFactory* GetViewNodeFactory();

private:
  struct vtkFrameInformation
  {
    vtkFrameInformation() = default;

    int ViewportX = 0;
    int ViewportY = 0;
    int ViewportWidth = 0;
    int ViewportHeight = 0;

    double TileViewport[4] = { 0.0, 0.0, 1.0, 1.0 };
    int TileScale[2] = { 1, 1 };
  };

  vtkAnariPass(const vtkAnariPass&) = delete;
  void operator=(const vtkAnariPass&) = delete;

  vtkAnariPass();
  ~vtkAnariPass() override = default;

  /**
   * Tells the pass what it will render.
   */
  void SetSceneGraph(vtkAnariSceneGraph*);

  void RenderAnariFrame(vtkRenderer* renderer, const vtkFrameInformation& frameInfo);

  void BlitAnariFrameToOpenGLFrame(vtkRenderer* renderer, const vtkFrameInformation& frameInfo);
  void SetupFrame(vtkOpenGLRenderWindow* openGLRenderWindow, vtkRenderer* renderer,
    const anari::Extensions& extensions);

  vtkSmartPointer<vtkAnariSceneGraph> SceneGraph;
  vtkNew<vtkAnariDevice> Device;
  vtkNew<vtkAnariRenderer> Renderer;
  vtkNew<vtkAnariViewNodeFactory> Factory;

  // ANARI frame blit to OpenGL
  std::unique_ptr<vtkOpenGLQuadHelper> OpenGLQuadHelper;
  vtkNew<vtkTextureObject> ColorTexture;
  vtkNew<vtkTextureObject> DepthTexture;
  vtkNew<vtkTextureObject> SharedColorTexture;
  vtkNew<vtkTextureObject> SharedDepthTexture;
};

VTK_ABI_NAMESPACE_END
#endif
