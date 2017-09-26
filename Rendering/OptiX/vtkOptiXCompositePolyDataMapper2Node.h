/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXCompositePolyDataMapper2Node.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOptiXCompositePolyDataMapper2Node
 * @brief   links vtkActor and vtkMapper to OptiX
 *
 * Translates vtkActor/Mapper state into OptiX rendering calls
*/

#ifndef vtkOptiXCompositePolyDataMapper2Node_h
#define vtkOptiXCompositePolyDataMapper2Node_h

#include "vtkRenderingOptiXModule.h" // For export macro
#include "vtkOptiXPolyDataMapperNode.h"
#include "vtkColor.h" // used for ivars
#include <stack> // used for ivars

class vtkDataObject;
class vtkCompositePolyDataMapper2;
class vtkOptiXRendererNode;

class VTKRENDERINGOPTIX_EXPORT vtkOptiXCompositePolyDataMapper2Node :
  public vtkOptiXPolyDataMapperNode
{
public:
  static vtkOptiXCompositePolyDataMapper2Node* New();
  vtkTypeMacro(vtkOptiXCompositePolyDataMapper2Node, vtkOptiXPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Make OptiX calls for rendering.
   */
  virtual void Render(bool prepass);

protected:
  vtkOptiXCompositePolyDataMapper2Node();
  ~vtkOptiXCompositePolyDataMapper2Node();

  class RenderBlockState
  {
  public:
    std::stack<bool> Visibility;
    std::stack<double> Opacity;
    std::stack<vtkColor3d> AmbientColor;
    std::stack<vtkColor3d> DiffuseColor;
    std::stack<vtkColor3d> SpecularColor;
  };

  void RenderBlock(vtkOptiXRendererNode *orn,
                   vtkCompositePolyDataMapper2 *cpdm,
                   vtkActor *actor,
                   vtkDataObject *dobj,
                   unsigned int &flat_index);

  RenderBlockState BlockState;

private:
  vtkOptiXCompositePolyDataMapper2Node(const vtkOptiXCompositePolyDataMapper2Node&) = delete;
  void operator=(const vtkOptiXCompositePolyDataMapper2Node&) = delete;
};
#endif
