/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayPointGaussianMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayPointGaussianMapperNode
 * @brief   links vtkActor and vtkMapper to OSPRay
 *
 * Translates vtkActor/Mapper state into OSPRay rendering calls
 */

#ifndef vtkOSPRayPointGaussianMapperNode_h
#define vtkOSPRayPointGaussianMapperNode_h

#include "vtkOSPRayCache.h" // For common cache infrastructure
#include "vtkPolyDataMapperNode.h"
#include "vtkRenderingRayTracingModule.h" // For export macro

class vtkOSPRayActorNode;
class vtkPolyData;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayPointGaussianMapperNode : public vtkPolyDataMapperNode
{
public:
  static vtkOSPRayPointGaussianMapperNode* New();
  vtkTypeMacro(vtkOSPRayPointGaussianMapperNode, vtkPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  void Invalidate(bool prepass) override;

protected:
  vtkOSPRayPointGaussianMapperNode();
  ~vtkOSPRayPointGaussianMapperNode() override;

  void ORenderPoly(void* renderer, vtkOSPRayActorNode* aNode, vtkPolyData* poly,
    double* ambientColor, double* diffuseColor, double opacity, std::string material);

  std::vector<OSPVolume> OSPRayVolumes;
  std::vector<OSPVolumetricModel> VolumetricModels;
  std::vector<OSPInstance> Instances;
  void ClearVolumetricModels();

  /**
   * @brief add precomputed ospray geometries to renderer model.
   */
  void RenderVolumetricModels();

private:
  vtkOSPRayPointGaussianMapperNode(const vtkOSPRayPointGaussianMapperNode&) = delete;
  void operator=(const vtkOSPRayPointGaussianMapperNode&) = delete;
};

#endif
