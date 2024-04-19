// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEdgeCenters
 * @brief   generate points at center of edges
 *
 * vtkEdgeCenters is a filter that takes as input any graph and
 * generates on output points at the center of the cells in the dataset.
 * These points can be used for placing glyphs (vtkGlyph3D) or labeling
 * (vtkLabeledDataMapper). (The center is the parametric center of the
 * cell, not necessarily the geometric or bounding box center.) The edge
 * attributes will be associated with the points on output.
 *
 * @warning
 * You can choose to generate just points or points and vertex cells.
 * Vertex cells are drawn during rendering; points are not. Use the ivar
 * VertexCells to generate cells.
 *
 * @sa
 * vtkGlyph3D vtkLabeledDataMapper
 */

#ifndef vtkEdgeCenters_h
#define vtkEdgeCenters_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkEdgeCenters : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkEdgeCenters, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with vertex cell generation turned off.
   */
  static vtkEdgeCenters* New();

  ///@{
  /**
   * Enable/disable the generation of vertex cells.
   */
  vtkSetMacro(VertexCells, vtkTypeBool);
  vtkGetMacro(VertexCells, vtkTypeBool);
  vtkBooleanMacro(VertexCells, vtkTypeBool);
  ///@}

protected:
  vtkEdgeCenters();
  ~vtkEdgeCenters() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkTypeBool VertexCells;

private:
  vtkEdgeCenters(const vtkEdgeCenters&) = delete;
  void operator=(const vtkEdgeCenters&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
