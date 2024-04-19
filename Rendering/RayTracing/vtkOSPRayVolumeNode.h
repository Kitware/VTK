// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayVolumeNode
 * @brief   links vtkVolume and vtkMapper to OSPRay
 *
 * Translates vtkVolume/Mapper state into OSPRay rendering calls
 */

#ifndef vtkOSPRayVolumeNode_h
#define vtkOSPRayVolumeNode_h

#include "vtkRenderingRayTracingModule.h" // For export macro
#include "vtkVolumeNode.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkVolume;
class vtkCompositeDataDisplayAttributes;
class vtkDataArray;
class vtkInformationIntegerKey;
class vtkInformationObjectBaseKey;
class vtkInformationStringKey;
class vtkPiecewiseFunction;
class vtkPolyData;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayVolumeNode : public vtkVolumeNode
{
public:
  static vtkOSPRayVolumeNode* New();
  vtkTypeMacro(vtkOSPRayVolumeNode, vtkVolumeNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to take into account my renderables time, including
   * mapper and data into mapper inclusive of composite input
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkOSPRayVolumeNode();
  ~vtkOSPRayVolumeNode() override;

private:
  vtkOSPRayVolumeNode(const vtkOSPRayVolumeNode&) = delete;
  void operator=(const vtkOSPRayVolumeNode&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
