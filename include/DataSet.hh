/*=========================================================================

  Program:   Visualization Library
  Module:    DataSet.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDataSet - abstract class to specify dataset behavior
// .SECTION Description
// vlDataSet is an abstract class that specifies an interface for 
// data objects. (Data objects are synomous with datasets). vlDataSet
// also provides methods to provide informations about the data such
// as center, bounding box, and representative length.

#ifndef __vlDataSet_h
#define __vlDataSet_h

#include "Object.hh"
#include "IdList.hh"
#include "FPoints.hh"
#include "PtData.hh"
#include "Cell.hh"

class vlDataSet : public vlObject 
{
public:
  vlDataSet();
  vlDataSet(const vlDataSet& ds);
  ~vlDataSet() {};
  char *GetClassName() {return "vlDataSet";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Restore data object to initial state (i.e., release memory, etc.).
  virtual void Initialize();

  // Description:
  // Provides opportunity for data to clean itself up before execution.
  virtual void Update() {};

  // Description:
  // Create concrete instance of this dataset.
  virtual vlDataSet *MakeObject() = 0;

  // Description:
  // Return class name of data type.
  virtual char *GetDataType() = 0;

  // Description:
  // Determine number of points composing dataset.
  virtual int GetNumberOfPoints() = 0;

  // Description:
  // Determine number of cells composing dataset.
  virtual int GetNumberOfCells() = 0;

  // Description:
  // Get point coordinates with ptId such that: 0 <= ptId < NumberOfPoints
  virtual float *GetPoint(int ptId) = 0;

  // Description:
  // Copy point coordinates into user provided array x[3] for specified
  // point id.
  virtual void GetPoint(int id, float x[3]);

  // Description:
  // Get cell with cellId such that: 0 <= cellId < NumberOfCells
  virtual vlCell *GetCell(int cellId) = 0;

  // Description:
  // Get type of cell with cellId such that: 0 <= cellId < NumberOfCells
  virtual int GetCellType(int cellId) = 0;

  // Description:
  // Topological inquiry to get points defining cell.
  virtual void GetCellPoints(int cellId, vlIdList& ptIds) = 0;

  // Description:
  // Topological inquiry to get cells using point.
  virtual void GetPointCells(int ptId, vlIdList& cellIds) = 0;

  // Description:
  // Topological inquiry to get all cells using list of points exclusive of
  // cell specified (e.g., cellId)
  virtual void GetCellNeighbors(int cellId, vlIdList& ptIds, vlIdList& cellIds);

  // Description:
  // Locate cell based on global coordinate x and tolerance squared. If
  // cell is non-NULL, then search starts from this cell and looks at 
  // immediate neighbors. Returns cellId >= 0 if inside, < 0 otherwise.
  // The parametric coordinates are provided in pcoords[3].
  virtual int FindCell(float x[3], vlCell *cell, float tol2, int& subId, float pcoords[3]) = 0;

  // some data sets are composite objects and need to check each part for MTime
  unsigned long int GetMTime();

  // compute geometric bounds, center, longest side
  virtual void ComputeBounds();
  float *GetBounds();
  void GetBounds(float bounds[6]);
  float *GetCenter();
  void GetCenter(float center[3]);
  float GetLength();

  // return pointer to this dataset's point data
  vlPointData *GetPointData() {return &this->PointData;};

  // Description:
  // Reclaim any extra memory used to store data.
  virtual void Squeeze();

protected:
  vlPointData PointData;   // Scalars, vectors, etc. associated w/ each point
  vlTimeStamp ComputeTime; // Time at which bounds, center, etc. computed
  float Bounds[6];  // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds
};

inline void vlDataSet::GetPoint(int id, float x[3])
{
  float *pt = this->GetPoint(id);
  x[0] = pt[0]; x[1] = pt[1]; x[2] = pt[2]; 
}

#endif


