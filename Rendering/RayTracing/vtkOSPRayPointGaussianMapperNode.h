// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayPointGaussianMapperNode
 * @brief   links vtkActor and vtkMapper to OSPRay
 *
 * Translates vtkActor/Mapper state into OSPRay rendering calls
 */

#ifndef vtkOSPRayPointGaussianMapperNode_h
#define vtkOSPRayPointGaussianMapperNode_h

#include "vtkOSPRayPolyDataMapperNode.h"
#include "vtkRenderingRayTracingModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOSPRayActorNode;
class vtkPolyData;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayPointGaussianMapperNode
  : public vtkOSPRayPolyDataMapperNode
{
public:
  static vtkOSPRayPointGaussianMapperNode* New();
  vtkTypeMacro(vtkOSPRayPointGaussianMapperNode, vtkOSPRayPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  void Render(bool prepass) override;

protected:
  vtkOSPRayPointGaussianMapperNode();
  ~vtkOSPRayPointGaussianMapperNode() override;

  void InternalRender(void* renderer, vtkOSPRayActorNode* aNode, vtkPolyData* poly, double opacity,
    std::string material);

  std::vector<OSPVolume> OSPRayVolumes;
  std::vector<OSPVolumetricModel> VolumetricModels;
  std::vector<OSPInstance> Instances;
  void ClearVolumetricModels();

  /**
   * @brief add precomputed ospray geometries to renderer model.
   */
  void RenderVolumetricModels();

  /**
   * @brief Do frame need to be rebuilt
   */
  bool GetNeedToRebuild(vtkOSPRayActorNode*);

  /**
   * Member variables
   */
  int NumColors = 128;
  int ScaleTableSize = 1024;
  float* ScaleTable = nullptr;
  double ScaleScale = 1.0;
  double ScaleOffset = 0.0;
  int OpacityTableSize = 1024;
  float* OpacityTable = nullptr;
  double OpacityScale = 1.0;
  double OpacityOffset = 0.0;
  vtkTimeStamp ScaleTableUpdateTime;
  vtkTimeStamp OpacityTableUpdateTime;

private:
  vtkOSPRayPointGaussianMapperNode(const vtkOSPRayPointGaussianMapperNode&) = delete;
  void operator=(const vtkOSPRayPointGaussianMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
