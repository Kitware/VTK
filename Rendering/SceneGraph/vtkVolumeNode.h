/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeNode
 * @brief   vtkViewNode specialized for vtkActors
 *
 * State storage and graph traversal for vtkActor/Mapper and Property
 * Made a choice to merge actor, mapper and property together. If there
 * is a compelling reason to separate them we can.
*/

#ifndef vtkVolumeNode_h
#define vtkVolumeNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

class VTKRENDERINGSCENEGRAPH_EXPORT vtkVolumeNode :
  public vtkViewNode
{
public:
  static vtkVolumeNode* New();
  vtkTypeMacro(vtkVolumeNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Build containers for our child nodes.
   */
  virtual void Build(bool prepass) override;

protected:
  vtkVolumeNode();
  ~vtkVolumeNode();

private:
  vtkVolumeNode(const vtkVolumeNode&) = delete;
  void operator=(const vtkVolumeNode&) = delete;
};

#endif
