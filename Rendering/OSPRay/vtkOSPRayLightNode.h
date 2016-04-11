/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayLightNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOSPRayLightNode - links vtkLights to OSPRay
// .SECTION Description
// Translates vtkLight state into OSPRay rendering calls

#ifndef vtkOSPRayLightNode_h
#define vtkOSPRayLightNode_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkLightNode.h"

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayLightNode :
  public vtkLightNode
{
public:
  static vtkOSPRayLightNode* New();
  vtkTypeMacro(vtkOSPRayLightNode, vtkLightNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Make ospray calls to render me.
  virtual void Render(bool prepass);

  //Description:
  //A global multiplier to all ospray lights.
  //default is 1.0
  static void SetLightScale(double s);
  static double GetLightScale();

protected:
  vtkOSPRayLightNode();
  ~vtkOSPRayLightNode();

private:
  vtkOSPRayLightNode(const vtkOSPRayLightNode&); // Not implemented.
  void operator=(const vtkOSPRayLightNode&); // Not implemented.

  static double LightScale;
};

#endif
