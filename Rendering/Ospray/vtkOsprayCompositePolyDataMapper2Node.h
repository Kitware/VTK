/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayCompositePolyDataMapper2Node.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOsprayCompositePolyDataMapper2Node - links vtkActor and vtkMapper to OSPRay
// .SECTION Description
// Translates vtkActor/Mapper state into OSPRay rendering calls

#ifndef vtkOsprayCompositePolyDataMapper2Node_h
#define vtkOsprayCompositePolyDataMapper2Node_h

#include "vtkRenderingOsprayModule.h" // For export macro
#include "vtkOsprayPolyDataMapperNode.h"
#include "vtkColor.h" // used for ivars
#include <stack> // used for ivars

class vtkDataObject;
class vtkCompositePolyDataMapper2;
class vtkOsprayRendererNode;

class VTKRENDERINGOSPRAY_EXPORT vtkOsprayCompositePolyDataMapper2Node :
  public vtkOsprayPolyDataMapperNode
{
public:
  static vtkOsprayCompositePolyDataMapper2Node* New();
  vtkTypeMacro(vtkOsprayCompositePolyDataMapper2Node, vtkOsprayPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Make ospray calls to render me.
  virtual void Render(bool prepass);

protected:
  vtkOsprayCompositePolyDataMapper2Node();
  ~vtkOsprayCompositePolyDataMapper2Node();

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
  void RenderBlock(vtkOsprayRendererNode *orn,
                   vtkCompositePolyDataMapper2 *cpdm,
                   vtkActor *actor,
                   vtkDataObject *dobj,
                   unsigned int &flat_index);


private:
  vtkOsprayCompositePolyDataMapper2Node(const vtkOsprayCompositePolyDataMapper2Node&); // Not implemented.
  void operator=(const vtkOsprayCompositePolyDataMapper2Node&); // Not implemented.
};
#endif
