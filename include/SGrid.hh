/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SGrid.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredGrid - topologically regular array of data
// .SECTION Description
// vtkStructuredGrid is a data object that is a concrete implementation of
// vtkDataSet. vtkStructuredGrid represents a geometric structure that is a
// topologically regular array of points. The topology is that of a cube that
// has been subdivided into a regular array of smaller cubes. Each point/cell
// can be addressed with i-j-k indices. Examples include finite difference 
// grids.

#ifndef __vtkStructuredGrid_h
#define __vtkStructuredGrid_h

#include "PointSet.hh"
#include "StrData.hh"

class vtkStructuredGrid : public vtkPointSet, public vtkStructuredData {
public:
  vtkStructuredGrid();
  vtkStructuredGrid(const vtkStructuredGrid& sg);
  ~vtkStructuredGrid();
  char *GetClassName() {return "vtkStructuredGrid";};
  char *GetDataType() {return "vtkStructuredGrid";};
  void PrintSelf(ostream& os, vtkIndent indent);
 
  unsigned long GetMtime();

  // dataset interface
  vtkDataSet *MakeObject() {return new vtkStructuredGrid(*this);};
  int GetNumberOfPoints() {return vtkPointSet::GetNumberOfPoints();};
  vtkCell *GetCell(int cellId);
  int GetCellType(int cellId);
  float *GetPoint(int ptId);
  void GetPoint(int ptId, float p[3]);
  int FindCell(float x[3], vtkCell *cell, float tol2, int& subId, 
               float pcoords[3],float weights[MAX_CELL_SIZE]);
  int GetNumberOfCells();
  void GetCellPoints(int cellId, vtkIdList& ptIds);
  void GetPointCells(int ptId, vtkIdList& cellIds);

protected:
  void Initialize();

  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  // blanking information inherited
};

inline float *vtkStructuredGrid::GetPoint(int ptId) 
{
  return this->vtkPointSet::GetPoint(ptId);
}

inline void vtkStructuredGrid::GetPoint(int ptId, float p[3]) 
{
  this->vtkPointSet::GetPoint(ptId,p);
}

inline int vtkStructuredGrid::GetNumberOfCells() 
{
  return this->vtkStructuredData::_GetNumberOfCells();
}

inline void vtkStructuredGrid::GetCellPoints(int cellId, vtkIdList& ptIds) 
{
  this->vtkStructuredData::_GetCellPoints(cellId,ptIds);
}

inline void vtkStructuredGrid::GetPointCells(int ptId, vtkIdList& cellIds) 
{
  this->vtkStructuredData::_GetPointCells(ptId,cellIds);
}

inline int vtkStructuredGrid::FindCell(float x[3], vtkCell *cell, float tol2, 
                                      int& subId, float pcoords[3],
                                      float weights[MAX_CELL_SIZE])
{
  return this->vtkPointSet::FindCell(x,cell,tol2,subId,pcoords,weights);
}

#endif




