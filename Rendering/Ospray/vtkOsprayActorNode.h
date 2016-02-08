/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayActorNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOsprayActorNode - links vtkActor and vtkMapper to OSPRay
// .SECTION Description
// Translates vtkActor/Mapper state into OSPRay rendering calls

#ifndef vtkOsprayActorNode_h
#define vtkOsprayActorNode_h

#include "vtkRenderingOsprayModule.h" // For export macro
#include "vtkActorNode.h"

class vtkActor;
class vtkPolyData;

class VTKRENDERINGOSPRAY_EXPORT vtkOsprayActorNode :
  public vtkActorNode
{
public:
  static vtkOsprayActorNode* New();
  vtkTypeMacro(vtkOsprayActorNode, vtkActorNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Make ospray calls to render me.
  virtual void RenderSelf();

  //Description:
  //todo - why?
  void ORender(void *oRenderer, void *oModel);

  //todo - why public?
  vtkTimeStamp RenderTime;

protected:
  vtkOsprayActorNode();
  ~vtkOsprayActorNode();

  void ORenderPoly(void *renderer, void *model,
                   vtkActor *act, vtkPolyData * poly);
  void *OSPMeshes;

private:
  vtkOsprayActorNode(const vtkOsprayActorNode&); // Not implemented.
  void operator=(const vtkOsprayActorNode&); // Not implemented.
};
#endif
