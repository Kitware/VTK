/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UGrid.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkUnstructuredGrid - dataset represents arbitrary combinations of all possible cell types
// .SECTION Description
// vtkUnstructuredGrid is a data object that is a concrete implementation 
// of vtkDataSet. vtkUnstructuredGrid represents any combinations of any cell
// types. This includes 0D (e.g., points), 1D (e.g., lines, polylines), 2D 
// (e.g., triangles, polygons), and 3D (e.g., hexahedron, tetrahedron).

#ifndef __vtkUnstructuredGrid_h
#define __vtkUnstructuredGrid_h

#include "PointSet.hh"
#include "IdList.hh"
#include "CellArr.hh"
#include "CellList.hh"
#include "LinkList.hh"

class vtkUnstructuredGrid : public vtkPointSet {
public:
  vtkUnstructuredGrid();
  vtkUnstructuredGrid(const vtkUnstructuredGrid& up);
  ~vtkUnstructuredGrid();
  char *GetClassName() {return "vtkUnstructuredGrid";};
  char *GetDataType() {return "vtkUnstructuredGrid";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // cell creation/manipulation methods
  void Allocate(int numCells=1000, int extSize=1000);
  int InsertNextCell(int type, vtkIdList& ptIds);
  int InsertNextCell(int type, int npts, int pts[MAX_CELL_SIZE]);
  void SetCells(int *types, vtkCellArray *cells);
  vtkCellArray *GetCells() {return this->Connectivity;};

  // dataset interface
  vtkDataSet *MakeObject() {return new vtkUnstructuredGrid(*this);};
  int GetNumberOfCells();
  vtkCell *GetCell(int cellId);
  void GetCellPoints(int cellId, vtkIdList& ptIds);
  void GetPointCells(int ptId, vtkIdList& cellIds);
  int GetCellType(int cellId);
  void Squeeze();

protected:
  void Initialize();

  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vtkCellList *Cells;
  vtkCellArray *Connectivity;
  vtkLinkList *Links;
  void BuildLinks();
};

#endif
