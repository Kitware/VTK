// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkOSPRayAMRVolumeMapperNode
 * @brief links vtkVolumeMapper  to OSPRay
 *
 * Translates vtkAMRVolumeMapper state into OSPRay rendering calls
 * Directly samples the vtkAMR data structure without resampling
 * Data is expected to be overlapping, only floats and doubles are now
 * supported.
 */

#ifndef vtkOSPRayAMRVolumeMapperNode_h
#define vtkOSPRayAMRVolumeMapperNode_h

#include "vtkOSPRayVolumeMapperNode.h"
#include "vtkRenderingRayTracingModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayAMRVolumeMapperNode : public vtkOSPRayVolumeMapperNode
{
public:
  static vtkOSPRayAMRVolumeMapperNode* New();
  vtkTypeMacro(vtkOSPRayAMRVolumeMapperNode, vtkOSPRayVolumeMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Traverse graph in ospray's preferred order and render
   */
  void Render(bool prepass) override;

protected:
  vtkOSPRayAMRVolumeMapperNode();
  ~vtkOSPRayAMRVolumeMapperNode() override = default;

private:
  vtkOSPRayAMRVolumeMapperNode(const vtkOSPRayAMRVolumeMapperNode&) = delete;
  void operator=(const vtkOSPRayAMRVolumeMapperNode&) = delete;

  float OldSamplingRate;
};
VTK_ABI_NAMESPACE_END
#endif
