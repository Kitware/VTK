/*=========================================================================

  Program:   Visualization Library
  Module:    SGrid.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredGrid - topologically regular array of data
// .SECTION Description
// vlStructuredGrid is a data object that is a concrete implementation of
// vlDataSet. vlStructuredGrid represents a geometric structure that is a
// topologically regular array of points. The topology is that of a cube that
// has been subdivided into a regular array of smaller cubes. Each point/cell
// can be addressed with i-j-k indices. Examples include finite difference 
// grids.

#ifndef __vlStructuredGrid_h
#define __vlStructuredGrid_h

#include "PointSet.hh"
#include "StrData.hh"

class vlStructuredGrid : public vlPointSet, public vlStructuredData {
public:
  vlStructuredGrid();
  vlStructuredGrid(const vlStructuredGrid& sg);
  ~vlStructuredGrid();
  char *GetClassName() {return "vlStructuredGrid";};
  char *GetDataType() {return "vlStructuredGrid";};
  void PrintSelf(ostream& os, vlIndent indent);
 
  unsigned long GetMtime();

  // dataset interface
  vlDataSet *MakeObject() {return new vlStructuredGrid(*this);};
  int GetNumberOfPoints() {return vlPointSet::GetNumberOfPoints();};
  vlCell *GetCell(int cellId);
  int GetCellType(int cellId);
  float *GetPoint(int ptId);
  void GetPoint(int ptId, float p[3]);
  int FindCell(float x[3], vlCell *cell, float tol2, int& subId, float pcoords[3]) { return this->vlPointSet::FindCell(x,cell,tol2,subId,pcoords);}
  int GetNumberOfCells();
  void GetCellPoints(int cellId, vlIdList& ptIds);
  void GetPointCells(int ptId, vlIdList& cellIds);

protected:
  void Initialize();

  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  // blanking information inherited
};

inline float *vlStructuredGrid::GetPoint(int ptId) 
{
  return this->vlPointSet::GetPoint(ptId);
}

inline void vlStructuredGrid::GetPoint(int ptId, float p[3]) 
{
  this->vlPointSet::GetPoint(ptId,p);
}

inline int vlStructuredGrid::GetNumberOfCells() 
{
  return this->vlStructuredData::_GetNumberOfCells();
}

inline void vlStructuredGrid::GetCellPoints(int cellId, vlIdList& ptIds) 
{
  this->vlStructuredData::_GetCellPoints(cellId,ptIds);
}

inline void vlStructuredGrid::GetPointCells(int ptId, vlIdList& cellIds) 
{
  this->vlStructuredData::_GetPointCells(ptId,cellIds);
}

#endif
