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

class vtkCollection;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkRendererNode : public vtkViewNode
{
public:
  static vtkRendererNode* New();
  vtkTypeMacro(vtkRendererNode, vtkViewNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Build containers for our child nodes.
   */
  void Build(bool prepass) override;

  /**
   * Get/Set the framebuffer size
   */
  vtkGetVector2Macro(Size, int);
  vtkSetVector2Macro(Size, int);

  /**
   * Get/Set the window viewport
   */
  vtkGetVector4Macro(Viewport, double);
  vtkSetVector4Macro(Viewport, double);

  /**
   * Get/Set the window tile scale
   */
  vtkGetVector2Macro(Scale, int);
  vtkSetVector2Macro(Scale, int);

protected:
  vtkRendererNode();
  ~vtkRendererNode() override;

  int Size[2];
  double Viewport[4];
  int Scale[2];

private:
  vtkRendererNode(const vtkRendererNode&) = delete;
  void operator=(const vtkRendererNode&) = delete;
};

#endif
