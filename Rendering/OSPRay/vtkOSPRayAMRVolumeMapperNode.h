/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayAMRVolumeMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkOSPRayVolumeMapperNode.h"

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayAMRVolumeMapperNode :
  public vtkOSPRayVolumeMapperNode
{
public:
  static vtkOSPRayAMRVolumeMapperNode* New();
  vtkTypeMacro(vtkOSPRayAMRVolumeMapperNode, vtkOSPRayVolumeMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
  * Traverse graph in ospray's preferred order and render
  */
  virtual void Render(bool prepass) override;
protected:

  vtkOSPRayAMRVolumeMapperNode();
  ~vtkOSPRayAMRVolumeMapperNode() = default;

private:
  vtkOSPRayAMRVolumeMapperNode(const vtkOSPRayAMRVolumeMapperNode&) = delete;
  void operator=(const vtkOSPRayAMRVolumeMapperNode&) = delete;

  float OldSamplingRate;
};
#endif
