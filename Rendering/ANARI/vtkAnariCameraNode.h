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

class VTKRENDERINGANARI_EXPORT vtkAnariCameraNode : public vtkCameraNode
{
public:
  static vtkAnariCameraNode* New();
  vtkTypeMacro(vtkAnariCameraNode, vtkCameraNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ANARI calls to render me.
   */
  virtual void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  virtual void Invalidate(bool prepass) override;

protected:
  vtkAnariCameraNode() = default;
  ~vtkAnariCameraNode() = default;

private:
  vtkAnariCameraNode(const vtkAnariCameraNode&) = delete;
  void operator=(const vtkAnariCameraNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
