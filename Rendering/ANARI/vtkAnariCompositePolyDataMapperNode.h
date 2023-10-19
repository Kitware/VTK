/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnariCompositePolyDataMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAnariCompositePolyDataMapperNode
 * @brief   links vtkActor and vtkMapper to ANARI
 *
 * Translates vtkActor/Mapper state into ANARI rendering calls
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariCompositePolyDataMapperNode_h
#define vtkAnariCompositePolyDataMapperNode_h

#include "vtkAnariPolyDataMapperNode.h"
#include "vtkColor.h"                // used for ivars
#include "vtkRenderingAnariModule.h" // For export macro

#include <stack> // used for ivars

VTK_ABI_NAMESPACE_BEGIN

class vtkDataObject;
class vtkCompositePolyDataMapper;

class VTKRENDERINGANARI_EXPORT vtkAnariCompositePolyDataMapperNode
  : public vtkAnariPolyDataMapperNode
{
public:
  static vtkAnariCompositePolyDataMapperNode* New();
  vtkTypeMacro(vtkAnariCompositePolyDataMapperNode, vtkAnariPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Make ANARI calls for rendering.
   */
  virtual void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  virtual void Invalidate(bool prepass) override;

protected:
  vtkAnariCompositePolyDataMapperNode() = default;
  ~vtkAnariCompositePolyDataMapperNode() = default;

  void RenderBlock(vtkCompositePolyDataMapper*, vtkActor*, vtkDataObject*, unsigned int&);

  class RenderBlockState
  {
  public:
    std::stack<bool> Visibility;
    std::stack<double> Opacity;
    std::stack<vtkColor3d> AmbientColor;
    std::stack<vtkColor3d> DiffuseColor;
    std::stack<vtkColor3d> SpecularColor;
    std::stack<std::string> Material;
  };

  RenderBlockState BlockState;

private:
  vtkAnariCompositePolyDataMapperNode(const vtkAnariCompositePolyDataMapperNode&) = delete;
  void operator=(const vtkAnariCompositePolyDataMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
