/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayMoleculeMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayMoleculeMapperNode
 * @brief   links vtkMoleculeMapper to OSPRay
 *
 * Translates vtkMoleculeMapper state into OSPRay rendering calls
 */

#ifndef vtkOSPRayMoleculeMapperNode_h
#define vtkOSPRayMoleculeMapperNode_h

#include "vtkPolyDataMapperNode.h"
#include "vtkRenderingRayTracingModule.h" // For export macro

#include "RTWrapper/RTWrapper.h" // for handle types
#include <vector>

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayMoleculeMapperNode : public vtkPolyDataMapperNode
{
public:
  static vtkOSPRayMoleculeMapperNode* New();
  vtkTypeMacro(vtkOSPRayMoleculeMapperNode, vtkPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass) override;

protected:
  vtkOSPRayMoleculeMapperNode();
  ~vtkOSPRayMoleculeMapperNode() override;

  vtkTimeStamp BuildTime;
  std::vector<OSPGeometry> Geometries;

private:
  vtkOSPRayMoleculeMapperNode(const vtkOSPRayMoleculeMapperNode&) = delete;
  void operator=(const vtkOSPRayMoleculeMapperNode&) = delete;
};
#endif
