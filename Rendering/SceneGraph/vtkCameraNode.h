// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCameraNode
 * @brief   vtkViewNode specialized for vtkCameras
 *
 * State storage and graph traversal for vtkCamera
 */

#ifndef vtkCameraNode_h
#define vtkCameraNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGSCENEGRAPH_EXPORT vtkCameraNode : public vtkViewNode
{
public:
  static vtkCameraNode* New();
  vtkTypeMacro(vtkCameraNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCameraNode();
  ~vtkCameraNode() override;

private:
  vtkCameraNode(const vtkCameraNode&) = delete;
  void operator=(const vtkCameraNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
