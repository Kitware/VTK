/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXCameraNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOptiXCameraNode
 * @brief   links vtkCamera to OptiX
 *
 * Translates vtkCamera state into OptiX rendering calls
*/

#ifndef vtkOptiXCameraNode_h
#define vtkOptiXCameraNode_h

#include "vtkRenderingOptiXModule.h" // For export macro
#include "vtkCameraNode.h"

class VTKRENDERINGOPTIX_EXPORT vtkOptiXCameraNode :
  public vtkCameraNode
{
public:
  static vtkOptiXCameraNode* New();
  vtkTypeMacro(vtkOptiXCameraNode, vtkCameraNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Make OptiX calls for rendering.
   */
  virtual void Render(bool prepass);

protected:
  vtkOptiXCameraNode();
  ~vtkOptiXCameraNode();

private:
  vtkOptiXCameraNode(const vtkOptiXCameraNode&) = delete;
  void operator=(const vtkOptiXCameraNode&) = delete;
};

#endif
