// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayUnstructuredVolumeMapperNode
 * @brief   Unstructured grid volume renderer.
 *
 * vtkOSPRayUnstructuredVolumeMapperNode implements a volume rendering
 * that directly samples the unstructured grid using OSPRay.
 *
 */

#ifndef vtkOSPRayUnstructuredVolumeMapperNode_h
#define vtkOSPRayUnstructuredVolumeMapperNode_h

#include "vtkOSPRayCache.h"               // For common cache infrastructure
#include "vtkRenderingRayTracingModule.h" // For export macro
#include "vtkVolumeMapperNode.h"

#include "RTWrapper/RTWrapper.h" // for handle types

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayUnstructuredVolumeMapperNode
  : public vtkVolumeMapperNode

{
public:
  vtkTypeMacro(vtkOSPRayUnstructuredVolumeMapperNode, vtkVolumeMapperNode);
  static vtkOSPRayUnstructuredVolumeMapperNode* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  void Render(bool prepass) override;

protected:
  vtkOSPRayUnstructuredVolumeMapperNode();
  ~vtkOSPRayUnstructuredVolumeMapperNode() = default;

  int NumColors;
  double SamplingRate;

  vtkTimeStamp BuildTime;
  vtkTimeStamp PropertyTime;

  OSPVolume OSPRayVolume;
  OSPVolumetricModel OSPRayVolumeModel;

  std::string LastArrayName = "";
  int LastArrayComponent = -2;

  OSPInstance OSPRayInstance{ nullptr };

private:
  vtkOSPRayUnstructuredVolumeMapperNode(const vtkOSPRayUnstructuredVolumeMapperNode&) = delete;
  void operator=(const vtkOSPRayUnstructuredVolumeMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
