/*=========================================================================

  Program:   Visualization Toolkit
  Module:    StrPts.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredPoints - topologically and geometrically regular array of data
// .SECTION Description
// vtkStructuredPoints is a data object that is a concrete implementation of
// vtkDataSet. vtkStructuredPoints represents a geometric structure that is 
// a topological and geometrical regular array of points. Examples include
// volumes (voxel data) and pixmaps. 

#ifndef __vtkStructuredPoints_h
#define __vtkStructuredPoints_h

#include "DataSet.hh"
#include "StrData.hh"

class vtkStructuredPoints : public vtkDataSet, public vtkStructuredData 
{
public:
  vtkStructuredPoints();
  vtkStructuredPoints(const vtkStructuredPoints& v);
  ~vtkStructuredPoints();
  char *GetClassName() {return "vtkStructuredPoints";};
  char *GetDataType() {return "vtkStructuredPoints";};
  void PrintSelf(ostream& os, vtkIndent indent);

  unsigned long GetMtime();

  // dataset interface
  vtkDataSet *MakeObject() {return new vtkStructuredPoints(*this);};
  int GetNumberOfCells();
  int GetNumberOfPoints();
  float *GetPoint(int ptId);
  void GetPoint(int id, float x[3]);
  vtkCell *GetCell(int cellId);
  int FindCell(float x[3], vtkCell *cell, float tol2, int& subId, 
               float pcoords[3], float weights[MAX_CELL_SIZE]);
  int GetCellType(int cellId);
  void GetCellPoints(int cellId, vtkIdList& ptIds);
  void GetPointCells(int ptId, vtkIdList& cellIds);
  void ComputeBounds();

  void GetVoxelGradient(int i, int j, int k, vtkScalars *s, vtkFloatVectors& g);
  void GetPointGradient(int i, int j, int k, vtkScalars *s, float g[3]);

  // Description:
  // Set the aspect ratio of the cubical cells that compose the structured
  // point set.
  vtkSetVector3Macro(AspectRatio,float);
  vtkGetVectorMacro(AspectRatio,float,3);

  // Description:
  // Set the origin of the data. The origin plus aspect ratio determine the
  // position in space of the structured points.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

protected:
  void Initialize();

  float Origin[3];
  float AspectRatio[3];
};

inline void vtkStructuredPoints::GetPoint(int id, float x[3])
{
  float *p=this->GetPoint(id);
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}

inline int vtkStructuredPoints::GetNumberOfCells() 
{
  return this->vtkStructuredData::_GetNumberOfCells();
}

inline int vtkStructuredPoints::GetNumberOfPoints()
{
  return this->vtkStructuredData::_GetNumberOfPoints();
}

inline void vtkStructuredPoints::GetCellPoints(int cellId, vtkIdList& ptIds)
{
  this->vtkStructuredData::_GetCellPoints(cellId, ptIds);
}

inline void vtkStructuredPoints::GetPointCells(int ptId, vtkIdList& cellIds)
{
  this->vtkStructuredData::_GetPointCells(ptId, cellIds);
}

#endif
