/*=========================================================================

  Program:   Visualization Library
  Module:    UGrid.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Unstructured Grid (i.e., arbitrary combinations of all possible cell types)
//
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
  void Allocate(int numCells=1000, int extSize=1000);
  char *GetClassName() {return "vlUnstructuredGrid";};
  void PrintSelf(ostream& os, vlIndent indent);

  // cell creation methods
  int InsertNextCell(int type, vlIdList& ptIds);
  int InsertNextCell(int type, int npts, int pts[MAX_CELL_SIZE]);
  void InsertCells(int numCells, int width, int* data);
  void InsertCells(int numCells, int* data);
  void Squeeze();

  // dataset interface
  vlDataSet *MakeObject() {return new vlUnstructuredGrid(*this);};
  vlMapper *MakeMapper();
  void Initialize();
  int GetNumberOfCells();
  vlCell *GetCell(int cellId);
  void GetCellPoints(int cellId, vlIdList *ptIds);
  void GetPointCells(int ptId, vlIdList *cellIds);
  int GetCellType(int cellId);

protected:
  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  vlCellList *Cells;
  vlCellArray *Connectivity;
  vlLinkList *Links;
  void BuildLinks();
};

#endif
