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
#include "vtkRenderingAnariModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariPassInternals;
class vtkAnariRendererNode;

class vtkCameraPass;
class vtkLightsPass;
class vtkOverlayPass;
class vtkRenderPassCollection;
class vtkSequencePass;
class vtkVolumetricPass;
class vtkViewNodeFactory;

class VTKRENDERINGANARI_EXPORT vtkAnariPass : public vtkRenderPass
{
public:
  static vtkAnariPass* New();
  vtkTypeMacro(vtkAnariPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Setup the trace directory and trace mode strings for the debug device for
   * when SetupAnariDeviceFromLibrary() is called. Once the Anari device is
   * created, this method will have no effect.
   */
  void SetAnariDebugConfig(const char* traceDir, const char* traceMode);

  /**
   * Initialize this vtkAnariPass from the name of an anari::Library and anari::Device
   * to be loaded. This initialization will use whatever debug configuration set
   * by SetupAnariDebugConfig() prior to this function when 'enableDebugLayer' is true.
   */
  void SetupAnariDeviceFromLibrary(
    const char* libraryName, const char* deviceName, bool enableDebugLayer = false);

  /**
   * Perform rendering according to a render state s.
   */
  virtual void Render(const vtkRenderState* s) override;

  //@{
  /**
   * Tells the pass what it will render.
   */
  void SetSceneGraph(vtkAnariRendererNode*);
  vtkGetObjectMacro(SceneGraph, vtkAnariRendererNode);
  //@}

private:
  /**
   * Default constructor.
   */
  vtkAnariPass();

  /**
   * Destructor.
   */
  ~vtkAnariPass() override;

  vtkAnariRendererNode* SceneGraph = nullptr;
  vtkNew<vtkCameraPass> CameraPass;

  vtkAnariPass(const vtkAnariPass&) = delete;
  void operator=(const vtkAnariPass&) = delete;

  friend class vtkAnariPassInternals;
  vtkAnariPassInternals* Internal = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif
