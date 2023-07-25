// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMapperNode
 * @brief   vtkViewNode specialized for vtkMappers
 *
 * State storage and graph traversal for vtkMapper
 */

#ifndef vtkMapperNode_h
#define vtkMapperNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

#include <vector> //for results

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkDataSet;
class vtkMapper;
class vtkPolyData;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkMapperNode : public vtkViewNode
{
public:
  static vtkMapperNode* New();
  vtkTypeMacro(vtkMapperNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkMapperNode();
  ~vtkMapperNode() override;

  vtkAbstractArray* GetArrayToProcess(vtkDataSet* input, int& cellFlag);

private:
  vtkMapperNode(const vtkMapperNode&) = delete;
  void operator=(const vtkMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
