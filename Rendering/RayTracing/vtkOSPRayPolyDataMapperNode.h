// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayPolyDataMapperNode
 * @brief   links vtkActor and vtkMapper to OSPRay
 *
 * Translates vtkActor/Mapper state into OSPRay rendering calls
 */

#ifndef vtkOSPRayPolyDataMapperNode_h
#define vtkOSPRayPolyDataMapperNode_h

#include "vtkOSPRayCache.h" // For common cache infrastructure
#include "vtkPolyDataMapperNode.h"
#include "vtkRenderingRayTracingModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOSPRayActorNode;
class vtkPolyData;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayPolyDataMapperNode : public vtkPolyDataMapperNode
{
public:
  static vtkOSPRayPolyDataMapperNode* New();
  vtkTypeMacro(vtkOSPRayPolyDataMapperNode, vtkPolyDataMapperNode);
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
  vtkOSPRayPolyDataMapperNode();
  ~vtkOSPRayPolyDataMapperNode() override;

  void ORenderPoly(void* renderer, vtkOSPRayActorNode* aNode, vtkPolyData* poly,
    double* ambientColor, double* diffuseColor, double opacity, std::string material);

  std::vector<OSPGeometricModel> GeometricModels;
  std::vector<OSPInstance> Instances;
  void ClearGeometricModels();

  /**
   * @brief add precomputed ospray geometries to renderer model.
   */
  void RenderGeometricModels();

private:
  vtkOSPRayPolyDataMapperNode(const vtkOSPRayPolyDataMapperNode&) = delete;
  void operator=(const vtkOSPRayPolyDataMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
