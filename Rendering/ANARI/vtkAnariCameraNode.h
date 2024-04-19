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
#include "vtkRenderingAnariModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariCameraNodeInternals;
class vtkAnariRendererNode;
class vtkCamera;

class VTKRENDERINGANARI_EXPORT vtkAnariCameraNode : public vtkCameraNode
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
  ~vtkAnariCameraNode();

private:
  vtkAnariCameraNode(const vtkAnariCameraNode&) = delete;
  void operator=(const vtkAnariCameraNode&) = delete;

  vtkCamera* GetVtkCamera() const;
  bool CameraWasModified() const;

  void UpdateAnariObjectHandles();
  void UpdateAnariCameraParameters();

  vtkAnariCameraNodeInternals* Internals{ nullptr };
};

VTK_ABI_NAMESPACE_END
#endif
