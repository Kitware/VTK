/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRendererNode - vtkViewNode specialized for vtkRenderers
// .SECTION Description
// State storage and graph traversal for vtkRenderer

#ifndef vtkRendererNode_h
#define vtkRendererNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

class VTKRENDERINGSCENEGRAPH_EXPORT vtkRendererNode :
  public vtkViewNode
{
public:
  static vtkRendererNode* New();
  vtkTypeMacro(vtkRendererNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Build containers for our child nodes.
  virtual void BuildSelf();

  //Description:
  //Get state of my renderable.
  virtual void SynchronizeSelf();

  //Description:
  //Override to interface to a specific backend.
  virtual void RenderSelf() {};

  //todo: need Set per VTK style checks
  int GetLayer() { return this->Layer; }

protected:
  vtkRendererNode();
  ~vtkRendererNode();

  //todo: use a map with string keys being renderable's member name
  //state
  double Ambient[3];
  double Background[3];
  double Background2[3];
  bool GradientBackground;
  int Layer;
  int Origin[2];
  int Size[2];
  int TiledOrigin[2];
  int TiledSize[2];

private:
  vtkRendererNode(const vtkRendererNode&); // Not implemented.
  void operator=(const vtkRendererNode&); // Not implemented.
};

#endif
