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
// .NAME vtkHyperTreeGridToUnstructuredGrid - Convert hyper tree grid to
// unstructured grid.
//
// .SECTION Description
// Make explicit all leaves of a hyper tree grid by converting them to cells
// of an unstructured grid.
// Produces segments in 1D, rectangles in 2D, right hexahedra in 3D.
// NB: The output will contain superimposed inter-element boundaries and pending
// nodes as a result of T-junctions.
//
// .SECTION See Also
// vtkHyperTreeGrid vtkUnstructuredGrid
//
// .SECTION Thanks
// This class was written by Philippe Pebay and Charles Law, Kitware 2012
// This work was supported in part by Commissariat a l'Energie Atomique (CEA/DIF)

#ifndef vtkHyperTreeGridToUnstructuredGrid_h
#define vtkHyperTreeGridToUnstructuredGrid_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkCellArray;
class vtkDataSetAttributes;
class vtkHyperTreeGrid;
class vtkPoints;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridToUnstructuredGrid : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkHyperTreeGridToUnstructuredGrid* New();
  vtkTypeMacro( vtkHyperTreeGridToUnstructuredGrid, vtkUnstructuredGridAlgorithm );
  void PrintSelf( ostream&, vtkIndent );

protected:
  vtkHyperTreeGridToUnstructuredGrid();
  ~vtkHyperTreeGridToUnstructuredGrid();

  unsigned int Dimension;
  unsigned int CellSize;
  unsigned int* Coefficients;

  virtual int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* );
  virtual int FillInputPortInformation( int, vtkInformation* );

  void ProcessTrees();
  void RecursiveProcessTree( void* );
  void AddCell( vtkIdType inId, double* origin, double* size );

  vtkHyperTreeGrid* Input;
  vtkUnstructuredGrid* Output;

  vtkDataSetAttributes* InData;
  vtkDataSetAttributes* OutData;

  vtkPoints* Points;
  vtkCellArray* Cells;

private:
  vtkHyperTreeGridToUnstructuredGrid(const vtkHyperTreeGridToUnstructuredGrid&);  // Not implemented.
  void operator=(const vtkHyperTreeGridToUnstructuredGrid&);  // Not implemented.
};

#endif
