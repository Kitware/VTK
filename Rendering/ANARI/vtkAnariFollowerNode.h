// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariFollowerNode
 * @brief   links vtkFollower to ANARI
 *
 * Translates vtkFollower state into ANARI state
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariFollowerNode_h
#define vtkAnariFollowerNode_h

#include "vtkAnariActorNode.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGANARI_EXPORT vtkAnariFollowerNode : public vtkAnariActorNode
{
public:
  static vtkAnariFollowerNode* New();
  vtkTypeMacro(vtkAnariFollowerNode, vtkAnariActorNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to take into account this renderables time, including
   * its associated camera
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkAnariFollowerNode() = default;
  ~vtkAnariFollowerNode() = default;

private:
  vtkAnariFollowerNode(const vtkAnariFollowerNode&) = delete;
  void operator=(const vtkAnariFollowerNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
