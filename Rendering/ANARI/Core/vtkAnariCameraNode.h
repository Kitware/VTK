// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariCameraNode
 * @brief   links vtkCamera to ANARI
 *
 * Translates vtkCamera state into ANARICamera state
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariCameraNode_h
#define vtkAnariCameraNode_h

#include "vtkCameraNode.h"
#include "vtkRenderingAnariCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

struct vtkAnariCameraNodeInternals;
class vtkAnariSceneGraph;
class vtkCamera;

class VTKRENDERINGANARICORE_EXPORT vtkAnariCameraNode : public vtkCameraNode
{
public:
  static vtkAnariCameraNode* New();
  vtkTypeMacro(vtkAnariCameraNode, vtkCameraNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Ensure the right type of ANARICamera object is being held.
   */
  void Build(bool prepass) override;

  /**
   * Sync ANARICamera parameters with vtkCamera.
   */
  void Synchronize(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  void Invalidate(bool prepass) override;

protected:
  vtkAnariCameraNode();
  ~vtkAnariCameraNode() override;

private:
  vtkAnariCameraNode(const vtkAnariCameraNode&) = delete;
  void operator=(const vtkAnariCameraNode&) = delete;

  /**
   * Return the camera object associated to the camera node.
   */
  vtkCamera* GetVtkCamera() const;

  /**
   * Return true if the camera from the node has a modified time higher than the render time.
   */
  bool CameraWasModified() const;

  /**
   * Update the internal ANARI camera handle.
   */
  void UpdateAnariObjectHandles();

  /**
   * Update camera parameters that have to change at all prepass.
   */
  void OnCameraPrePass();

  /**
   * Update camera parameters that change when CameraWasModified returns true.
   */
  void OnCameraModified();

  vtkAnariCameraNodeInternals* Internals{ nullptr };
};

VTK_ABI_NAMESPACE_END
#endif
