// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayMoleculeMapperNode
 * @brief   links vtkMoleculeMapper to OSPRay
 *
 * Translates vtkMoleculeMapper state into OSPRay rendering calls
 */

#ifndef vtkOSPRayMoleculeMapperNode_h
#define vtkOSPRayMoleculeMapperNode_h

#include "vtkPolyDataMapperNode.h"
#include "vtkRenderingRayTracingModule.h" // For export macro

#include "RTWrapper/RTWrapper.h" // for handle types
#include <vector>                // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayMoleculeMapperNode : public vtkPolyDataMapperNode
{
public:
  static vtkOSPRayMoleculeMapperNode* New();
  vtkTypeMacro(vtkOSPRayMoleculeMapperNode, vtkPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  void Render(bool prepass) override;

protected:
  vtkOSPRayMoleculeMapperNode();
  ~vtkOSPRayMoleculeMapperNode() override;

  vtkTimeStamp BuildTime;
  std::vector<OSPGeometricModel> GeometricModels;
  std::vector<OSPInstance> Instances;

private:
  vtkOSPRayMoleculeMapperNode(const vtkOSPRayMoleculeMapperNode&) = delete;
  void operator=(const vtkOSPRayMoleculeMapperNode&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
