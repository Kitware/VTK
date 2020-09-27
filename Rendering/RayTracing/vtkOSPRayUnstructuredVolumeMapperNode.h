/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayUnstructuredVolumeMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#endif
