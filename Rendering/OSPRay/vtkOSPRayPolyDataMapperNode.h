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

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkPolyDataMapperNode.h"

class vtkOSPRayActorNode;
class vtkPolyData;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayPolyDataMapperNode :
  public vtkPolyDataMapperNode
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
  ~vtkOSPRayPolyDataMapperNode();

  void ORenderPoly(void *renderer, void *model,
                   vtkOSPRayActorNode *aNode, vtkPolyData * poly,
                   double *ambientColor,
                   double *diffuseColor,
                   double opacity,
                   std::string material);

  void *OSPMeshes;
  void CreateNewMeshes();
  void AddMeshesToModel(void *arg);

private:
  vtkOSPRayPolyDataMapperNode(const vtkOSPRayPolyDataMapperNode&) = delete;
  void operator=(const vtkOSPRayPolyDataMapperNode&) = delete;
};
#endif
