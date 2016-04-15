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
// .NAME vtkOSPRayVolumeMapperNode - links vtkActor and vtkMapper to OSPRay
// .SECTION Description
// Translates vtkActor/Mapper state into OSPRay rendering calls

#ifndef vtkOSPRayVolumeMapperNode_h
#define vtkOSPRayVolumeMapperNode_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkVolumeMapperNode.h"

#include <map> //TODO hide in implementation to keep out of header

class vtkOSPRayActorNode;
class vtkPolyData;
class vtkVolume;

namespace osp
{
  struct TransferFunction;
  struct Volume;
//   struct Model;
}

struct vtkOSPRayVolumeCacheEntry
{
  osp::Volume* Volume;
  vtkTimeStamp BuildTime;
};


class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayVolumeMapperNode :
  public vtkVolumeMapperNode
{
public:
  static vtkOSPRayVolumeMapperNode* New();
  vtkTypeMacro(vtkOSPRayVolumeMapperNode, vtkVolumeMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent);
  virtual void Render(bool prepass);
protected:
  vtkOSPRayVolumeMapperNode();
  ~vtkOSPRayVolumeMapperNode();

  // The distance between sample points along the ray
  int CellFlag;
  double                       SampleDistance;
  double                       ImageSampleDistance;
  double                       MinimumImageSampleDistance;
  double                       MaximumImageSampleDistance;
  int                          AutoAdjustSampleDistances;

  int                          ScalarDataType;
  void                         *ScalarDataPointer;

  int           IntermixIntersectingGeometry;

  float        *ZBuffer;

  int NumberOfThreads;
  // vtkOSPRayManager *OSPRayManager;
  osp::Volume* OSPRayVolume;
  // osp::Model* OSPRayModel;
  vtkTimeStamp  BuildTime,PropertyTime;
  osp::TransferFunction* transferFunction;
  int NumColors;
  std::vector<float> TFVals, TFOVals;
  bool SharedData;
  bool VolumeAdded;
  double SamplingRate;
  std::map< vtkVolume*, std::map< double, vtkOSPRayVolumeCacheEntry* > > Cache;

private:
  vtkOSPRayVolumeMapperNode(const vtkOSPRayVolumeMapperNode&); // Not implemented.
  void operator=(const vtkOSPRayVolumeMapperNode&); // Not implemented.
};
#endif
