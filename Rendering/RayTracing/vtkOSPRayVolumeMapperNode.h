// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayVolumeMapperNode
 * @brief   links vtkVolumeMapper  to OSPRay
 *
 * Translates vtkVolumeMapper state into OSPRay rendering calls
 */

#ifndef vtkOSPRayVolumeMapperNode_h
#define vtkOSPRayVolumeMapperNode_h

#include "vtkOSPRayCache.h"               // For common cache infrastructure
#include "vtkRenderingRayTracingModule.h" // For export macro
#include "vtkVolumeMapperNode.h"

#include "RTWrapper/RTWrapper.h" // for handle types

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataSet;
class vtkVolume;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayVolumeMapperNode : public vtkVolumeMapperNode
{
public:
  static vtkOSPRayVolumeMapperNode* New();
  vtkTypeMacro(vtkOSPRayVolumeMapperNode, vtkVolumeMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  void Render(bool prepass) override;

  /**
   * TODO: fix me
   * should be controlled by VTK SampleDistance, otherwise
   * should use macros and modify self.
   */
  void SetSamplingRate(double rate) { this->SamplingRate = rate; }
  double GetSamplingRate() { return this->SamplingRate; }

protected:
  vtkOSPRayVolumeMapperNode();
  ~vtkOSPRayVolumeMapperNode() override;

  /**
   * updates internal OSPRay transfer function for volume
   */
  void UpdateTransferFunction(RTW::Backend* be, vtkVolume* vol, double* dataRange = nullptr);

  // TODO: SetAndGetters?
  int NumColors;
  double SamplingRate;
  double SamplingStep; // base sampling step of each voxel

  vtkTimeStamp BuildTime;
  vtkTimeStamp PropertyTime;
  vtkDataArray* LastArray;
  int LastComponent;

  OSPVolume OSPRayVolume;
  OSPVolumetricModel OSPRayVolumeModel;
  OSPGeometricModel Cropper;
  OSPTransferFunction TransferFunction;
  OSPInstance OSPRayInstance;

  std::vector<float> TFVals;
  std::vector<float> TFOVals;
  std::vector<float> IsoColors;

  vtkOSPRayCache<vtkOSPRayCacheItemObject>* Cache;

private:
  vtkOSPRayVolumeMapperNode(const vtkOSPRayVolumeMapperNode&) = delete;
  void operator=(const vtkOSPRayVolumeMapperNode&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
