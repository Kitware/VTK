/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXLightNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOptiXLightNode
 * @brief   links vtkLights to OptiX
 *
 * Translates vtkLight state into OptiX rendering calls
*/

#ifndef vtkOptiXLightNode_h
#define vtkOptiXLightNode_h

#include "vtkRenderingOptiXModule.h" // For export macro
#include "vtkLightNode.h"

class VTKRENDERINGOPTIX_EXPORT vtkOptiXLightNode :
  public vtkLightNode
{
public:
  static vtkOptiXLightNode* New();
  vtkTypeMacro(vtkOptiXLightNode, vtkLightNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Make OptiX calls for rendering.
   */
  virtual void Render(bool prepass);

  //@{
  /**
   * A global multiplier to all OptiX lights.
   * default is 1.0
   */
  static void SetLightScale(double s);
  static double GetLightScale();
  //@}

protected:
  vtkOptiXLightNode();
  ~vtkOptiXLightNode();

private:
  vtkOptiXLightNode(const vtkOptiXLightNode&) = delete;
  void operator=(const vtkOptiXLightNode&) = delete;

  static double LightScale;
};

#endif
