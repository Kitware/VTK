// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariPolyDataMapperNode
 * @brief   links vtkActor and vtkMapper to ANARI
 *
 * Parse VTK data and properties into the appropriate ANARI objects
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariPolyDataMapperNode_h
#define vtkAnariPolyDataMapperNode_h

#include "vtkPolyDataMapperNode.h"
#include "vtkRenderingAnariModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkActor;
class vtkAnariPolyDataMapperNodeInternals;
class vtkAnariActorNode;
class vtkPolyData;
class vtkDataSetSurfaceFilter;
class vtkAnariRendererNode;

class VTKRENDERINGANARI_EXPORT vtkAnariPolyDataMapperNode : public vtkPolyDataMapperNode
{
public:
  static vtkAnariPolyDataMapperNode* New();
  vtkTypeMacro(vtkAnariPolyDataMapperNode, vtkPolyDataMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Ensure this node has been intialized.
   */
  void Build(bool prepass) override;
  /**
   * Sync ANARIGeometry + ANARIMaterial parameters with vtkPolyData.
   */
  void Synchronize(bool prepass) override;
  /**
   * Make ANARI calls to render me.
   */
  void Render(bool prepass) override;
  /**
   * Invalidates cached rendering data.
   */
  void Invalidate(bool prepass) override;

protected:
  vtkAnariPolyDataMapperNode();
  ~vtkAnariPolyDataMapperNode();

  vtkActor* GetVtkActor() const;
  vtkAnariActorNode* GetAnariActorNode() const;
  bool NodeWasModified() const;
  void RenderSurfaceModels(bool);
  void ClearSurfaces();

  void AnariRenderPoly(vtkAnariActorNode* anariActorNode, vtkPolyData* poly, double* diffuse,
    double opacity, const std::string& materialName);
  void SetAnariConfig(vtkAnariRendererNode*);

  vtkAnariPolyDataMapperNodeInternals* Internal;

private:
  vtkAnariPolyDataMapperNode(const vtkAnariPolyDataMapperNode&) = delete;
  void operator=(const vtkAnariPolyDataMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
