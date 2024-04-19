// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayWindowNode
 * @brief   links vtkRendererWindows to OSPRay
 *
 * Translates vtkRenderWindow state into OSPRay rendering calls
 */

#ifndef vtkOSPRayWindowNode_h
#define vtkOSPRayWindowNode_h

#include "vtkRenderingRayTracingModule.h" // For export macro
#include "vtkWindowNode.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayWindowNode : public vtkWindowNode
{
public:
  static vtkOSPRayWindowNode* New();
  vtkTypeMacro(vtkOSPRayWindowNode, vtkWindowNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  void Render(bool prepass) override;

protected:
  vtkOSPRayWindowNode();
  ~vtkOSPRayWindowNode() override;

private:
  vtkOSPRayWindowNode(const vtkOSPRayWindowNode&) = delete;
  void operator=(const vtkOSPRayWindowNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
