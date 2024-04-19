// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellCenters
 * @brief   generate points at center of cells
 *
 * vtkCellCenters is a filter that takes as input any dataset and
 * generates on output points at the center of the cells in the dataset.
 * These points can be used for placing glyphs (vtkGlyph3D) or labeling
 * (vtkLabeledDataMapper). (The center is the parametric center of the
 * cell, not necessarily the geometric or bounding box center.) The cell
 * attributes will be associated with the points on output.
 *
 * @warning
 * You can choose to generate just points or points and vertex cells.
 * Vertex cells are drawn during rendering; points are not. Use the ivar
 * VertexCells to generate cells.
 *
 * @note
 * Empty cells will be ignored but will require a one by one cell to
 * point data copy that will make the processing slower.
 *
 * @sa
 * vtkGlyph3D vtkLabeledDataMapper
 */

#ifndef vtkCellCenters_h
#define vtkCellCenters_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;

class VTKFILTERSCORE_EXPORT vtkCellCenters : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkCellCenters, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with vertex cell generation turned off.
   */
  static vtkCellCenters* New();

  ///@{
  /**
   * Enable/disable the generation of vertex cells. The default
   * is Off.
   */
  vtkSetMacro(VertexCells, bool);
  vtkGetMacro(VertexCells, bool);
  vtkBooleanMacro(VertexCells, bool);
  ///@}

  ///@{
  /**
   * Enable/disable whether input cell data arrays should be passed through (or
   * copied) as output point data arrays. Default is `true` i.e. the arrays will
   * be propagated.
   */
  vtkSetMacro(CopyArrays, bool);
  vtkGetMacro(CopyArrays, bool);
  vtkBooleanMacro(CopyArrays, bool);
  ///@}

  ///@{
  /**
   * Enable/disable whether the ghost cells are converted into ghost points.
   * For example, if ON, the input HIDDENCELLS and REFINEDCELLS are converted into HIDDENPOINTS.
   * If OFF, the ghost array is just passed to the output as well as the `GhostsToSkip` from
   * the input `vtkCellData`.
   * It is ON by default.
   *
   * @sa vtkFieldData
   */
  vtkSetMacro(ConvertGhostCellsToGhostPoints, bool);
  vtkGetMacro(ConvertGhostCellsToGhostPoints, bool);
  vtkBooleanMacro(ConvertGhostCellsToGhostPoints, bool);
  ///@}

  /**
   * Compute centers of cells from a dataset, storing them in the centers array.
   */
  static void ComputeCellCenters(vtkDataSet* dataset, vtkDoubleArray* centers);

protected:
  vtkCellCenters() = default;
  ~vtkCellCenters() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  bool VertexCells = false;
  bool CopyArrays = true;
  bool ConvertGhostCellsToGhostPoints = true;

private:
  vtkCellCenters(const vtkCellCenters&) = delete;
  void operator=(const vtkCellCenters&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
