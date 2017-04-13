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
/**
 * @class   vtkRendererNode
 * @brief   vtkViewNode specialized for vtkRenderers
 *
 * State storage and graph traversal for vtkRenderer
*/

#ifndef vtkRendererNode_h
#define vtkRendererNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkViewNode.h"

class  vtkCollection;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkRendererNode :
  public vtkViewNode
{
public:
  static vtkRendererNode* New();
  vtkTypeMacro(vtkRendererNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Build containers for our child nodes.
   */
  virtual void Build(bool prepass) VTK_OVERRIDE;

  /**
   * Synchronize our state
   */
  virtual void Synchronize(bool prepass) VTK_OVERRIDE;

protected:
  vtkRendererNode();
  ~vtkRendererNode();

  int Size[2];

private:
  vtkRendererNode(const vtkRendererNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRendererNode&) VTK_DELETE_FUNCTION;
};

#endif
