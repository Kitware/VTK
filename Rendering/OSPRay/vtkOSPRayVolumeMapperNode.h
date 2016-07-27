/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayVolumeMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOSPRayVolumeMapperNode - links vtkVolumeMapper  to OSPRay
// .SECTION Description
// Translates vtkVolumeMapper state into OSPRay rendering calls

#ifndef vtkOSPRayVolumeMapperNode_h
#define vtkOSPRayVolumeMapperNode_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkVolumeMapperNode.h"

namespace osp
{
  struct TransferFunction;
  struct Volume;
}

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayVolumeMapperNode :
  public vtkVolumeMapperNode
{
public:
  static vtkOSPRayVolumeMapperNode* New();
  vtkTypeMacro(vtkOSPRayVolumeMapperNode, vtkVolumeMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //
  virtual void Render(bool prepass);

protected:
  vtkOSPRayVolumeMapperNode();
  ~vtkOSPRayVolumeMapperNode();

  //TODO: SetAndGetters?
  int NumColors;
  bool SharedData;
  double SamplingRate;

  osp::Volume* OSPRayVolume;

  vtkTimeStamp BuildTime;
  vtkTimeStamp PropertyTime;
  osp::TransferFunction* TransferFunction;
  std::vector<float> TFVals, TFOVals;

private:
  vtkOSPRayVolumeMapperNode(const vtkOSPRayVolumeMapperNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSPRayVolumeMapperNode&) VTK_DELETE_FUNCTION;
};
#endif
