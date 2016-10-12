/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayCameraNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayCameraNode
 * @brief   links vtkCamera to OSPRay
 *
 * Translates vtkCamera state into OSPRay rendering calls
*/

#ifndef vtkOSPRayCameraNode_h
#define vtkOSPRayCameraNode_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkCameraNode.h"

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayCameraNode :
  public vtkCameraNode
{
public:
  static vtkOSPRayCameraNode* New();
  vtkTypeMacro(vtkOSPRayCameraNode, vtkCameraNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass);

protected:
  vtkOSPRayCameraNode();
  ~vtkOSPRayCameraNode();

private:
  vtkOSPRayCameraNode(const vtkOSPRayCameraNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSPRayCameraNode&) VTK_DELETE_FUNCTION;
};

#endif
