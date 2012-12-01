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

#ifndef __vtkHyperTreeGridToUnstructuredGrid_h
#define __vtkHyperTreeGridToUnstructuredGrid_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkHyperTreeGrid.h" // We need this because of supercursor

class vtkPoints;
class vtkCellArray;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridToUnstructuredGrid : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkHyperTreeGridToUnstructuredGrid* New();
  vtkTypeMacro(vtkHyperTreeGridToUnstructuredGrid,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkHyperTreeGridToUnstructuredGrid();
  ~vtkHyperTreeGridToUnstructuredGrid();

  unsigned int Dimension;
  unsigned int CellSize;
  unsigned int* Coefficients;

  virtual int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* );
  virtual int FillInputPortInformation( int, vtkInformation* );

  void ProcessTrees();
  void RecursiveProcessTree(vtkHyperTreeGridSuperCursor* superCursor);
  void AddCell( vtkIdType inId, double* origin, double* size );

  vtkHyperTreeGrid* Input;
  vtkUnstructuredGrid* Output;
  vtkPoints* Points;
  vtkCellArray* Cells;

private:
  vtkHyperTreeGridToUnstructuredGrid(const vtkHyperTreeGridToUnstructuredGrid&);  // Not implemented.
  void operator=(const vtkHyperTreeGridToUnstructuredGrid&);  // Not implemented.
};

#endif
