/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLightNode
 * @brief   vtkViewNode specialized for vtkLights
 *
 * State storage and graph traversal for vtkLight
*/

#ifndef vtkLightNode_h
#define vtkLightNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

class VTKRENDERINGSCENEGRAPH_EXPORT vtkLightNode :
  public vtkViewNode
{
public:
  static vtkLightNode* New();
  vtkTypeMacro(vtkLightNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkLightNode();
  ~vtkLightNode();

private:
  vtkLightNode(const vtkLightNode&) = delete;
  void operator=(const vtkLightNode&) = delete;
};

#endif
