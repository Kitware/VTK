// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOSPRayCompositePolyDataMapperNode
 * @brief   links vtkActor and vtkMapper to OSPRay
 *
 * Translates vtkActor/Mapper state into OSPRay rendering calls
 */

#ifndef vtkOSPRayCompositePolyDataMapperNode_h
#define vtkOSPRayCompositePolyDataMapperNode_h

#include "vtkColor.h" // used for ivars
#include "vtkOSPRayPolyDataMapperNode.h"
#include "vtkRenderingRayTracingModule.h" // For export macro
#include <stack>                          // used for ivars

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkCompositePolyDataMapper;
class vtkOSPRayRendererNode;

class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayCompositePolyDataMapperNode
  : public vtkOSPRayPolyDataMapperNode
{
public:
  static vtkOSPRayCompositePolyDataMapperNode* New();
  vtkTypeMacro(vtkOSPRayCompositePolyDataMapperNode, vtkOSPRayPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  void Invalidate(bool prepass) override;

protected:
  vtkOSPRayCompositePolyDataMapperNode();
  ~vtkOSPRayCompositePolyDataMapperNode() override;

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
  void RenderBlock(vtkOSPRayRendererNode* orn, vtkCompositePolyDataMapper* cpdm, vtkActor* actor,
    vtkDataObject* dobj, unsigned int& flat_index);

private:
  vtkOSPRayCompositePolyDataMapperNode(const vtkOSPRayCompositePolyDataMapperNode&) = delete;
  void operator=(const vtkOSPRayCompositePolyDataMapperNode&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
