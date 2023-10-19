/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnariPolyDataMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
   * Make ANARI calls to render me.
   */
  virtual void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  virtual void Invalidate(bool prepass) override;

protected:
  vtkAnariPolyDataMapperNode();
  ~vtkAnariPolyDataMapperNode();

  void RenderSurfaceModels(const bool);
  void ClearSurfaces();

  void AnariRenderPoly(vtkAnariActorNode* const anariActorNode, vtkPolyData* const poly,
    double* const diffuse, const double opacity, const std::string materialName);
  void SetAnariConfig(vtkAnariRendererNode* const);

  vtkAnariPolyDataMapperNodeInternals* Internal;

private:
  vtkAnariPolyDataMapperNode(const vtkAnariPolyDataMapperNode&) = delete;
  void operator=(const vtkAnariPolyDataMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
