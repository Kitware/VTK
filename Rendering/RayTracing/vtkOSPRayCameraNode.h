// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayCameraNode
 * @brief   links vtkCamera to OSPRay
 *
 * Translates vtkCamera state into OSPRay rendering calls
 */

#ifndef vtkOSPRayCameraNode_h
#define vtkOSPRayCameraNode_h

#include "vtkCameraNode.h"
#include "vtkRenderingRayTracingModule.h" // For export macro

#include "RTWrapper/RTWrapper.h" // for handle types

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationIntegerKey;
class vtkCamera;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayCameraNode : public vtkCameraNode
{
public:
  static vtkOSPRayCameraNode* New();
  vtkTypeMacro(vtkOSPRayCameraNode, vtkCameraNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  void Render(bool prepass) override;

  OSPCamera GetOCamera() { return this->oCamera; }

protected:
  vtkOSPRayCameraNode();
  ~vtkOSPRayCameraNode() override;

  OSPCamera oCamera{ nullptr };

private:
  vtkOSPRayCameraNode(const vtkOSPRayCameraNode&) = delete;
  void operator=(const vtkOSPRayCameraNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
