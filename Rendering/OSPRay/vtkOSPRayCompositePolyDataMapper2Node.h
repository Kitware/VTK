/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayCompositePolyDataMapper2Node.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayCompositePolyDataMapper2Node
 * @brief   links vtkActor and vtkMapper to OSPRay
 *
 * Translates vtkActor/Mapper state into OSPRay rendering calls
*/

#ifndef vtkOSPRayCompositePolyDataMapper2Node_h
#define vtkOSPRayCompositePolyDataMapper2Node_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkOSPRayPolyDataMapperNode.h"
#include "vtkColor.h" // used for ivars
#include <stack> // used for ivars

class vtkDataObject;
class vtkCompositePolyDataMapper2;
class vtkOSPRayRendererNode;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayCompositePolyDataMapper2Node :
  public vtkOSPRayPolyDataMapperNode
{
public:
  static vtkOSPRayCompositePolyDataMapper2Node* New();
  vtkTypeMacro(vtkOSPRayCompositePolyDataMapper2Node, vtkOSPRayPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass) VTK_OVERRIDE;

  /**
   * Invalidates cached rendering data.
   */
  virtual void Invalidate(bool prepass);

protected:
  vtkOSPRayCompositePolyDataMapper2Node();
  ~vtkOSPRayCompositePolyDataMapper2Node();

    class RenderBlockState
    {
    public:
      std::stack<bool> Visibility;
      std::stack<double> Opacity;
      std::stack<vtkColor3d> AmbientColor;
      std::stack<vtkColor3d> DiffuseColor;
      std::stack<vtkColor3d> SpecularColor;
    };

  RenderBlockState BlockState;
  void RenderBlock(vtkOSPRayRendererNode *orn,
                   vtkCompositePolyDataMapper2 *cpdm,
                   vtkActor *actor,
                   vtkDataObject *dobj,
                   unsigned int &flat_index);


private:
  vtkOSPRayCompositePolyDataMapper2Node(const vtkOSPRayCompositePolyDataMapper2Node&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSPRayCompositePolyDataMapper2Node&) VTK_DELETE_FUNCTION;
};
#endif
