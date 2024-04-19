// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLightNode
 * @brief   vtkViewNode specialized for vtkLights
 *
 * State storage and graph traversal for vtkLight
 */

#ifndef vtkLightNode_h
#define vtkLightNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGSCENEGRAPH_EXPORT vtkLightNode : public vtkViewNode
{
public:
  static vtkLightNode* New();
  vtkTypeMacro(vtkLightNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkLightNode();
  ~vtkLightNode() override;

private:
  vtkLightNode(const vtkLightNode&) = delete;
  void operator=(const vtkLightNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
