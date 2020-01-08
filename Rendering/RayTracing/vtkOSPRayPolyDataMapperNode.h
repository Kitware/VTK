/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayPolyDataMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  virtual void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  virtual void Invalidate(bool prepass) override;

protected:
  vtkOSPRayPolyDataMapperNode();
  ~vtkOSPRayPolyDataMapperNode() override;

  void ORenderPoly(void* renderer, vtkOSPRayActorNode* aNode, vtkPolyData* poly,
    double* ambientColor, double* diffuseColor, double opacity, std::string material);

  class vtkOSPRayCacheItemGeometries
  {
  public:
    vtkOSPRayCacheItemGeometries() = default;
    vtkOSPRayCacheItemGeometries(const std::vector<OSPGeometry>& geometries_)
      : GeometriesAtTime(geometries_)
    {
    }

    ~vtkOSPRayCacheItemGeometries() = default;

    std::vector<OSPGeometry> GeometriesAtTime;
  };

  std::vector<OSPGeometry> Geometries;
  void ClearGeometries();

  vtkOSPRayCache<vtkOSPRayCacheItemGeometries>* GeometryCache{ nullptr };
  vtkOSPRayCache<vtkOSPRayCacheItemObject>* InstanceCache{ nullptr };

  /**
   * @brief adds geometries to ospray cache
   */
  void PopulateCache();

  /**
   * @brief add computed ospray geometries to renderer model.
   * Will grab from cache if cached.
   */
  void RenderGeometries();

  bool UseInstanceCache;
  bool UseGeometryCache;

private:
  vtkOSPRayPolyDataMapperNode(const vtkOSPRayPolyDataMapperNode&) = delete;
  void operator=(const vtkOSPRayPolyDataMapperNode&) = delete;
};

#endif
