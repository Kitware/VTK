/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMapperNode
 * @brief   vtkViewNode specialized for vtkMappers
 *
 * State storage and graph traversal for vtkMapper
*/

#ifndef vtkMapperNode_h
#define vtkMapperNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

#include <vector> //for results

class vtkMapper;
class vtkPolyData;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkMapperNode :
  public vtkViewNode
{
public:
  static vtkMapperNode* New();
  vtkTypeMacro(vtkMapperNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkMapperNode();
  ~vtkMapperNode();

 private:
  vtkMapperNode(const vtkMapperNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMapperNode&) VTK_DELETE_FUNCTION;
};

#endif
