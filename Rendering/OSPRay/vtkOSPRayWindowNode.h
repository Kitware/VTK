/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayWindowNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayWindowNode
 * @brief   links vtkRendererWindows to OSPRay
 *
 * Translates vtkRenderWindow state into OSPRay rendering calls
*/

#ifndef vtkOSPRayWindowNode_h
#define vtkOSPRayWindowNode_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkWindowNode.h"

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayWindowNode :
  public vtkWindowNode
{
public:
  static vtkOSPRayWindowNode* New();
  vtkTypeMacro(vtkOSPRayWindowNode, vtkWindowNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass) override;

protected:
  vtkOSPRayWindowNode();
  ~vtkOSPRayWindowNode();

private:
  vtkOSPRayWindowNode(const vtkOSPRayWindowNode&) = delete;
  void operator=(const vtkOSPRayWindowNode&) = delete;
};

#endif
