/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayCameraNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOsprayCameraNode - links vtkCamera to OSPRay
// .SECTION Description
// Translates vtkCamera state into OSPRay rendering calls

#ifndef vtkOsprayCameraNode_h
#define vtkOsprayCameraNode_h

#include "vtkRenderingOsprayModule.h" // For export macro
#include "vtkCameraNode.h"

class VTKRENDERINGOSPRAY_EXPORT vtkOsprayCameraNode :
  public vtkCameraNode
{
public:
  static vtkOsprayCameraNode* New();
  vtkTypeMacro(vtkOsprayCameraNode, vtkCameraNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Make ospray calls to render me.
  virtual void Render(bool prepass);

protected:
  vtkOsprayCameraNode();
  ~vtkOsprayCameraNode();

private:
  vtkOsprayCameraNode(const vtkOsprayCameraNode&); // Not implemented.
  void operator=(const vtkOsprayCameraNode&); // Not implemented.
};

#endif
