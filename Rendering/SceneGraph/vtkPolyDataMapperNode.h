// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataMapperNode
 * @brief   vtkViewNode specialized for vtkPolyDataMappers
 *
 * State storage and graph traversal for vtkPolyDataMapper/PolyDataMapper and Property
 * Made a choice to merge PolyDataMapper, PolyDataMapper and property together. If there
 * is a compelling reason to separate them we can.
 */

#ifndef vtkPolyDataMapperNode_h
#define vtkPolyDataMapperNode_h

#include "vtkMapperNode.h"
#include "vtkRenderingSceneGraphModule.h" // For export macro

#include <vector> //for results

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkPolyDataMapper;
class vtkPolyData;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkPolyDataMapperNode : public vtkMapperNode
{
public:
  static vtkPolyDataMapperNode* New();
  vtkTypeMacro(vtkPolyDataMapperNode, vtkMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  struct vtkPDConnectivity_t
  {
    std::vector<unsigned int> vertex_index;
    std::vector<unsigned int> vertex_reverse;
    std::vector<unsigned int> line_index;
    std::vector<unsigned int> line_reverse;
    std::vector<unsigned int> triangle_index;
    std::vector<unsigned int> triangle_reverse;
    std::vector<unsigned int> strip_index;
    std::vector<unsigned int> strip_reverse;
  };
  using vtkPDConnectivity = struct vtkPDConnectivity_t;

protected:
  vtkPolyDataMapperNode();
  ~vtkPolyDataMapperNode() override;

  // Utilities for children
  /**
   * Makes a cleaned up version of the polydata's geometry in which NaN are removed
   * (substituted with neighbor) and the PolyDataMapper's transformation matrix is applied.
   */
  static void TransformPoints(vtkActor* act, vtkPolyData* poly, std::vector<double>& vertices);

  /**
   * Homogenizes the entire polydata using internal CreateXIndexBuffer functions.
   * They flatten the input polydata's Points, Lines, Polys, and Strips contents into
   * the output arrays. The output "index" arrays contain indices into the Points. The
   * output "reverse" array contains indices into the original CellArray.
   */
  static void MakeConnectivity(vtkPolyData* poly, int representation, vtkPDConnectivity& conn);

private:
  vtkPolyDataMapperNode(const vtkPolyDataMapperNode&) = delete;
  void operator=(const vtkPolyDataMapperNode&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
