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

#include "vtkAnariDevice.h"   // For vtkAnariDevice
#include "vtkAnariRenderer.h" // For vtkAnariRenderer

VTK_ABI_NAMESPACE_BEGIN

// Forward declarations
class vtkAnariPassInternals;
class vtkAnariSceneGraph;
class vtkCameraPass;
class vtkViewNodeFactory;

class VTKRENDERINGANARI_EXPORT vtkAnariPass : public vtkRenderPass
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
  /**
   * Default constructor.
   */
  vtkAnariPass();

  /**
   * Destructor.
   */
  ~vtkAnariPass() override;

  /**
   * Tells the pass what it will render.
   */
  void SetSceneGraph(vtkAnariSceneGraph*);

  vtkAnariSceneGraph* SceneGraph = nullptr;
  vtkNew<vtkCameraPass> CameraPass;

  vtkAnariPass(const vtkAnariPass&) = delete;
  void operator=(const vtkAnariPass&) = delete;

  friend class vtkAnariPassInternals;
  vtkAnariPassInternals* Internal = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif
