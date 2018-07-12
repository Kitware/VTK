/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayTetrahedraMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayTetrahedraMapperNode
 * @brief   Unstructured grid volume renderer.
 *
 * vtkOSPRayTetrahedraMapperNode implements a volume rendering
 * that directly samples the AMR structure using OSPRay.
 *
*/

#ifndef vtkOSPRayTetrahedraMapperNode_h
#define vtkOSPRayTetrahedraMapperNode_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkVolumeMapperNode.h"

#include "ospray/ospray.h" // for ospray handle types

class vtkOSPRayVolumeCache;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayTetrahedraMapperNode : public vtkVolumeMapperNode

{
public:
  vtkTypeMacro(vtkOSPRayTetrahedraMapperNode,
                       vtkVolumeMapperNode);
  static vtkOSPRayTetrahedraMapperNode *New();
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass) override;

protected:
  vtkOSPRayTetrahedraMapperNode();
  ~vtkOSPRayTetrahedraMapperNode() override;

  int NumColors;
  double SamplingRate;

  vtkTimeStamp BuildTime;
  vtkTimeStamp PropertyTime;

  OSPVolume OSPRayVolume;
  OSPTransferFunction TransferFunction;
  std::vector<float> TFVals;
  std::vector<float> TFOVals;

  std::vector<int> Cells;
  std::vector<osp::vec3f> Vertices;
  std::vector<float> Field;

  vtkOSPRayVolumeCache *Cache;
private:
  vtkOSPRayTetrahedraMapperNode(const vtkOSPRayTetrahedraMapperNode&) = delete;
  void operator=(const vtkOSPRayTetrahedraMapperNode &) = delete;
};

#endif
