// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVolumeMapperNode
 * @brief   vtkViewNode specialized for vtkVolumeMappers
 *
 * State storage and graph traversal for vtkVolumeMapper/PolyDataMapper and Property
 * Made a choice to merge PolyDataMapper, PolyDataMapper and property together. If there
 * is a compelling reason to separate them we can.
 */

#ifndef vtkVolumeMapperNode_h
#define vtkVolumeMapperNode_h

#include "vtkMapperNode.h"
#include "vtkRenderingSceneGraphModule.h" // For export macro

#include <vector> //for results

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkVolumeMapper;
class vtkPolyData;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkVolumeMapperNode : public vtkMapperNode
{
public:
  static vtkVolumeMapperNode* New();
  vtkTypeMacro(vtkVolumeMapperNode, vtkMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkVolumeMapperNode();
  ~vtkVolumeMapperNode() override;

private:
  vtkVolumeMapperNode(const vtkVolumeMapperNode&) = delete;
  void operator=(const vtkVolumeMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
