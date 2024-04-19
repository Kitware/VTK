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

  /**
   * Called by the internals of this class
   */
  virtual void RenderInternal(const vtkRenderState* s);

  /**
   * Get the view node factory that makes ANARI specific translator
   * instances for every VTK rendering pipeline class instance it
   * encounters.
   */
  virtual vtkViewNodeFactory* GetViewNodeFactory();

protected:
  /**
   * Default constructor.
   */
  vtkAnariPass();

  /**
   * Destructor.
   */
  virtual ~vtkAnariPass();

  vtkAnariRendererNode* SceneGraph;
  vtkNew<vtkCameraPass> CameraPass;

private:
  vtkAnariPass(const vtkAnariPass&) = delete;
  void operator=(const vtkAnariPass&) = delete;

  vtkAnariPassInternals* Internal;
  const char* PreviousRendererSubtype;
};

VTK_ABI_NAMESPACE_END
#endif
