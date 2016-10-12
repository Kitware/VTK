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
/**
 * @class   vtkOSPRayLightNode
 * @brief   links vtkLights to OSPRay
 *
 * Translates vtkLight state into OSPRay rendering calls
*/

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

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass);

  //@{
  /**
   * A global multiplier to all ospray lights.
   * default is 1.0
   */
  static void SetLightScale(double s);
  static double GetLightScale();
  //@}

protected:
  vtkOSPRayLightNode();
  ~vtkOSPRayLightNode();

private:
  vtkOSPRayLightNode(const vtkOSPRayLightNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSPRayLightNode&) VTK_DELETE_FUNCTION;

  static double LightScale;
};

#endif
