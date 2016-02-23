/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayWindowNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOsprayWindowNode - links vtkRendererWindows to OSPRay
// .SECTION Description
// Translates vtkRenderWindow state into OSPRay rendering calls

#ifndef vtkOsprayWindowNode_h
#define vtkOsprayWindowNode_h

#include "vtkRenderingOsprayModule.h" // For export macro
#include "vtkWindowNode.h"

class VTKRENDERINGOSPRAY_EXPORT vtkOsprayWindowNode :
  public vtkWindowNode
{
public:
  static vtkOsprayWindowNode* New();
  vtkTypeMacro(vtkOsprayWindowNode, vtkWindowNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Make ospray calls to render me.
  virtual void Render(bool prepass);

protected:
  vtkOsprayWindowNode();
  ~vtkOsprayWindowNode();

private:
  vtkOsprayWindowNode(const vtkOsprayWindowNode&); // Not implemented.
  void operator=(const vtkOsprayWindowNode&); // Not implemented.
};

#endif
