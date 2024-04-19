// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkActorNode
 * @brief   vtkViewNode specialized for vtkActors
 *
 * State storage and graph traversal for vtkActor/Mapper and Property
 * Made a choice to merge actor, mapper and property together. If there
 * is a compelling reason to separate them we can.
 */

#ifndef vtkActorNode_h
#define vtkActorNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGSCENEGRAPH_EXPORT vtkActorNode : public vtkViewNode
{
public:
  static vtkActorNode* New();
  vtkTypeMacro(vtkActorNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Build containers for our child nodes.
   */
  void Build(bool prepass) override;

protected:
  vtkActorNode();
  ~vtkActorNode() override;

private:
  vtkActorNode(const vtkActorNode&) = delete;
  void operator=(const vtkActorNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
