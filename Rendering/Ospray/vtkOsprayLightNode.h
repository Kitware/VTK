/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayLightNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOsprayLightNode - links vtkLights to OSPRay
// .SECTION Description
// Translates vtkLight state into OSPRay rendering calls

#ifndef vtkOsprayLightNode_h
#define vtkOsprayLightNode_h

#include "vtkRenderingOsprayModule.h" // For export macro
#include "vtkLightNode.h"

class VTKRENDERINGOSPRAY_EXPORT vtkOsprayLightNode :
  public vtkLightNode
{
public:
  static vtkOsprayLightNode* New();
  vtkTypeMacro(vtkOsprayLightNode, vtkLightNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Make ospray calls to render me.
  virtual void ORender(void *oRenderer);

protected:
  vtkOsprayLightNode();
  ~vtkOsprayLightNode();

private:
  vtkOsprayLightNode(const vtkOsprayLightNode&); // Not implemented.
  void operator=(const vtkOsprayLightNode&); // Not implemented.
};

#endif
