// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVolumeNode
 * @brief   vtkViewNode specialized for vtkActors
 *
 * State storage and graph traversal for vtkActor/Mapper and Property
 * Made a choice to merge actor, mapper and property together. If there
 * is a compelling reason to separate them we can.
 */

#ifndef vtkVolumeNode_h
#define vtkVolumeNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGSCENEGRAPH_EXPORT vtkVolumeNode : public vtkViewNode
{
public:
  static vtkVolumeNode* New();
  vtkTypeMacro(vtkVolumeNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Build containers for our child nodes.
   */
  void Build(bool prepass) override;

protected:
  vtkVolumeNode();
  ~vtkVolumeNode() override;

private:
  vtkVolumeNode(const vtkVolumeNode&) = delete;
  void operator=(const vtkVolumeNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
