/*=========================================================================

  Program:   Visualization Library
  Module:    DataSet.hh
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
// Abstract class for specifying dataset behaviour
//
#ifndef __vlDataSet_h
#define __vlDataSet_h

#include "Object.hh"
#include "IdList.hh"
#include "FPoints.hh"
#include "PtData.hh"
#include "Mapper.hh"
#include "Cell.hh"

class vlDataSet : virtual public vlObject 
{
public:
  vlDataSet();
  char *GetClassName() {return "vlDataSet";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Instantiate appropriate mapper object for this dataset
  virtual vlMapper *MakeMapper() = 0;
  // restore data to initial state (i.e., release memory, etc.)
  virtual void Initialize();
  // absorb update methods which propagate through network
  virtual void Update() {};

  // Create concrete instance of this dataset
  virtual vlDataSet *MakeObject() = 0;

  // Determine number of points and cells composing dataset
  virtual int GetNumberOfPoints() = 0;
  virtual int GetNumberOfCells() = 0;

  // Get point or cell of id 0<=cellId<NumberOfPoints/Cells
  virtual float *GetPoint(int ptId) = 0;
  virtual vlCell *GetCell(int cellId) = 0;
  virtual int GetCellType(int cellId) = 0;

  // topological inquiries to get neighbors and cells that use a point
  virtual void GetCellPoints(int cellId, vlIdList *ptIds) = 0;
  virtual void GetPointCells(int ptId, vlIdList *cellIds) = 0;
  virtual void GetCellNeighbors(int cellId, vlIdList *ptIds, 
                                vlIdList *cellIds);

  // Locate cell based on global coordinate x and tolerance squared.  If cell 
  // is non-Null, then search starts from this cell and looks at 
  // immediate neighbors. Returns cellId >= 0 if inside, < 0 otherwise.  The
  // parametric coords are provided in pcoords[3].
  virtual int FindCell(float x[3], vlCell *cell, float tol2, 
                       int& subId, float pcoords[3]) = 0;

  // some data sets are composite objects and need to check each part for MTime
  unsigned long int GetMTime();

  // compute geometric bounds, center, longest side
  virtual void ComputeBounds();
  float *GetBounds();
  float *GetCenter();
  float GetLength();
  
  // return pointer to this dataset's point data
  vlPointData *GetPointData() {return &this->PointData;};

protected:
  vlPointData PointData;   // Scalars, vectors, etc. associated w/ each point
  vlTimeStamp ComputeTime; // Time at which bounds, center, etc. computed
  float Bounds[6];  // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds
  vlMapper *Mapper; // object used to map data to graphics
};

#endif


