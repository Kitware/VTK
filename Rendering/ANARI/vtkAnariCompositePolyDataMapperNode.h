// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

class vtkActor;
class vtkMapper;
class vtkDataObject;
class vtkCompositePolyDataMapper;
class vtkCompositeDataDisplayAttributes;

class VTKRENDERINGANARI_EXPORT vtkAnariCompositePolyDataMapperNode
  : public vtkAnariPolyDataMapperNode
{
public:
  static vtkAnariCompositePolyDataMapperNode* New();
  vtkTypeMacro(vtkAnariCompositePolyDataMapperNode, vtkAnariPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Sync VTK and ANARI objects.
   */
  void Synchronize(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  void Invalidate(bool prepass) override;

protected:
  vtkAnariCompositePolyDataMapperNode() = default;

  void SynchronizeBlock(vtkMapper*, vtkActor*, vtkDataObject*, unsigned int&);

  virtual vtkCompositeDataDisplayAttributes* GetCompositeDisplayAttributes();

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
