// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridCellCenters
 * @brief   generate points at center of hyper
 * tree grid leaf cell centers.
 *
 *
 * vtkHyperTreeGridCellCenters is a filter that takes as input an hyper
 * tree grid and generates on output points at the center of the leaf
 * cells in the hyper tree grid.
 * These points can be used for placing glyphs (vtkGlyph3D) or labeling
 * (vtkLabeledDataMapper).
 * The cell attributes will be associated with the points on output.
 *
 * @warning
 * You can choose to generate just points or points and vertex cells.
 * Vertex cells are drawn during rendering; points are not. Use the ivar
 * VertexCells to generate cells.
 *
 * @sa
 * vtkCellCenters vtkHyperTreeGrid vtkGlyph3D
 *
 * @par Thanks:
 * This class was written by Guenole Harel and Jacques-Bernard Lekien 2014
 * This class was modified by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien, 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridCellCenters_h
#define vtkHyperTreeGridCellCenters_h

#include "vtkCellCenters.h"
#include "vtkFiltersHyperTreeModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class vtkDataSetAttributes;
class vtkHyperTreeGrid;
class vtkPolyData;
class vtkHyperTreeGridNonOrientedGeometryCursor;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridCellCenters : public vtkCellCenters
{
public:
  static vtkHyperTreeGridCellCenters* New();
  vtkTypeMacro(vtkHyperTreeGridCellCenters, vtkCellCenters);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkHyperTreeGridCellCenters();
  ~vtkHyperTreeGridCellCenters() override;

  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Main routine to process individual trees in the grid
   */
  virtual void ProcessTrees();

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree(vtkHyperTreeGridNonOrientedGeometryCursor*);

  vtkHyperTreeGrid* Input;
  vtkPolyData* Output;

  vtkDataSetAttributes* InData;
  vtkDataSetAttributes* OutData;

  vtkPoints* Points;

  vtkPointData* InPointData;
  vtkPointData* OutPointData;

  vtkBitArray* InMask;

private:
  vtkHyperTreeGridCellCenters(const vtkHyperTreeGridCellCenters&) = delete;
  void operator=(const vtkHyperTreeGridCellCenters&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridCellCenters_h
