/*=========================================================================

  Program:   Visualization Library
  Module:    UGrid.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlUnstructuredGrid - dataset represents arbitrary combinations of all possible cell types
// .SECTION Description
// vlUnstructuredGrid is a data object that is a concrete implementation 
// of vlDataSet. vlUnstructuredGrid represents any combinations of any cell
// types. This includes 0D (e.g., points), 1D (e.g., lines, polylines), 2D 
// (e.g., triangles, polygons), and 3D (e.g., hexahedron, tetrahedron).

#ifndef __vlUnstructuredGrid_h
#define __vlUnstructuredGrid_h

#include "PointSet.hh"
#include "IdList.hh"
#include "CellArr.hh"
#include "CellList.hh"
#include "LinkList.hh"

class vlUnstructuredGrid : public vlPointSet {
public:
  vlUnstructuredGrid();
  vlUnstructuredGrid(const vlUnstructuredGrid& up);
  ~vlUnstructuredGrid();
  char *GetClassName() {return "vlUnstructuredGrid";};
  char *GetDataType() {return "vlUnstructuredGrid";};
  void PrintSelf(ostream& os, vlIndent indent);

  // cell creation methods
  void Allocate(int numCells=1000, int extSize=1000);
  int InsertNextCell(int type, vlIdList& ptIds);
  int InsertNextCell(int type, int npts, int pts[MAX_CELL_SIZE]);
  void InsertCells(int numCells, int width, int* data);
  void InsertCells(int numCells, int* data);

  // dataset interface
  vlDataSet *MakeObject() {return new vlUnstructuredGrid(*this);};
  int GetNumberOfCells();
  vlCell *GetCell(int cellId);
  void GetCellPoints(int cellId, vlIdList& ptIds);
  void GetPointCells(int ptId, vlIdList& cellIds);
  int GetCellType(int cellId);
  void Squeeze();

protected:
  void Initialize();

  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vlCellList *Cells;
  vlCellArray *Connectivity;
  vlLinkList *Links;
  void BuildLinks();
};

#endif
