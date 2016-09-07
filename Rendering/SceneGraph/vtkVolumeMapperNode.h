/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVolumeMapperNode - vtkViewNode specialized for vtkVolumeMappers
// .SECTION Description
// State storage and graph traversal for vtkVolumeMapper/PolyDataMapper and Property
// Made a choice to merge PolyDataMapper, PolyDataMapper and property together. If there
// is a compelling reason to separate them we can.

#ifndef vtkVolumeMapperNode_h
#define vtkVolumeMapperNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkMapperNode.h"

#include <vector> //for results

class vtkActor;
class vtkVolumeMapper;
class vtkPolyData;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkVolumeMapperNode :
  public vtkMapperNode
{
public:
  static vtkVolumeMapperNode* New();
  vtkTypeMacro(vtkVolumeMapperNode, vtkMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkVolumeMapperNode();
  ~vtkVolumeMapperNode();

 private:
  vtkVolumeMapperNode(const vtkVolumeMapperNode&); // Not implemented.
  void operator=(const vtkVolumeMapperNode&); // Not implemented.
};

#endif
