/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridToUnstructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * This class was modified by Guénolé Harel and Jacques-Bernard Lekien, 2014
 * This class was rewritten by Philippe Pebay, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridToUnstructuredGrid_h
#define vtkHyperTreeGridToUnstructuredGrid_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkBitArray;
class vtkCellArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridCursor;
class vtkPoints;
class vtkUnstructuredGrid;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridToUnstructuredGrid : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridToUnstructuredGrid* New();
  vtkTypeMacro( vtkHyperTreeGridToUnstructuredGrid, vtkHyperTreeGridAlgorithm );
  void PrintSelf( ostream&, vtkIndent ) override;

protected:
  vtkHyperTreeGridToUnstructuredGrid();
  ~vtkHyperTreeGridToUnstructuredGrid() override;

  /**
   * For this algorithm the output is a vtkUnstructuredGrid instance
   */
  int FillOutputPortInformation( int, vtkInformation* ) override;

  /**
   * Main routine to convert the grid of tree into an unstructured grid
   */
  int ProcessTrees( vtkHyperTreeGrid*, vtkDataObject* ) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Helper method to generate a 2D or 3D cell
   */
  void AddCell( vtkIdType, double*, double* );

  /**
   * Storage for points of output unstructured mesh
   */
  vtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  vtkCellArray* Cells;

  /**
   * Storage for dimension of underlying tree
   */
  unsigned int Dimension;

private:
  vtkHyperTreeGridToUnstructuredGrid(const vtkHyperTreeGridToUnstructuredGrid&) = delete;
  void operator=(const vtkHyperTreeGridToUnstructuredGrid&) = delete;
};

#endif /* vtkHyperTreeGridToUnstructuredGrid_h */
