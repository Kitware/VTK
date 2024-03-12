// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridToUnstructuredGrid
 * @brief   Convert hyper tree grid to
 * unstructured grid.
 *
 * Make explicit all leaves of a hyper tree grid by converting them to cells
 * of an unstructured grid.
 * Produces segments in 1D, rectangles in 2D, right hexahedra in 3D.
 * NB: The output will contain superimposed inter-element boundaries and pending
 * nodes as a result of T-junctions.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2012
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien, 2014
 * This class was rewritten by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien, 2018
 * This class was corriged (used orientation) by Jacques-Bernard Lekien, 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridToUnstructuredGrid_h
#define vtkHyperTreeGridToUnstructuredGrid_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkCellArray;
class vtkHyperTreeGrid;
class vtkIdTypeArray;
class vtkPoints;
class vtkUnstructuredGrid;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridToUnstructuredGrid
  : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridToUnstructuredGrid* New();
  vtkTypeMacro(vtkHyperTreeGridToUnstructuredGrid, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Add a cell array with original HTG ids
   */
  vtkGetMacro(AddOriginalIds, bool);
  vtkSetMacro(AddOriginalIds, bool);
  vtkBooleanMacro(AddOriginalIds, bool);
  ///@}

protected:
  vtkHyperTreeGridToUnstructuredGrid();
  ~vtkHyperTreeGridToUnstructuredGrid() override;

  /**
   * For this algorithm the output is a vtkUnstructuredGrid instance
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to convert the grid of tree into an unstructured grid
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(vtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Helper method to generate a 2D or 3D cell
   */
  void AddCell(vtkIdType, double*, double*);

  /**
   * Storage for points of output unstructured mesh
   */
  vtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  vtkCellArray* Cells;

  /**
   * Storage of underlying tree
   */
  unsigned int Dimension;
  unsigned int Orientation;
  const unsigned int* Axes;

  bool AddOriginalIds;
  vtkIdTypeArray* OriginalIds;

private:
  vtkHyperTreeGridToUnstructuredGrid(const vtkHyperTreeGridToUnstructuredGrid&) = delete;
  void operator=(const vtkHyperTreeGridToUnstructuredGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridToUnstructuredGrid_h */
