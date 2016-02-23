/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayPolyDataMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOsprayPolyDataMapperNode - links vtkActor and vtkMapper to OSPRay
// .SECTION Description
// Translates vtkActor/Mapper state into OSPRay rendering calls

#ifndef vtkOsprayPolyDataMapperNode_h
#define vtkOsprayPolyDataMapperNode_h

#include "vtkRenderingOsprayModule.h" // For export macro
#include "vtkPolyDataMapperNode.h"

class vtkOsprayActorNode;
class vtkPolyData;

class VTKRENDERINGOSPRAY_EXPORT vtkOsprayPolyDataMapperNode :
  public vtkPolyDataMapperNode
{
public:
  static vtkOsprayPolyDataMapperNode* New();
  vtkTypeMacro(vtkOsprayPolyDataMapperNode, vtkPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Make ospray calls to render me.
  virtual void Render(bool prepass);

protected:
  vtkOsprayPolyDataMapperNode();
  ~vtkOsprayPolyDataMapperNode();

  void ORenderPoly(void *renderer, void *model,
                   vtkOsprayActorNode *aNode, vtkPolyData * poly,
                   double *ambientColor,
                   double *diffuseColor,
                   double opacity);

  void *OSPMeshes;
  void CreateNewMeshes();
  void AddMeshesToModel(void *arg);

private:
  vtkOsprayPolyDataMapperNode(const vtkOsprayPolyDataMapperNode&); // Not implemented.
  void operator=(const vtkOsprayPolyDataMapperNode&); // Not implemented.
};
#endif
