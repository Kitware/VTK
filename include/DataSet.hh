/*=========================================================================

  Program:   Visualization Library
  Module:    DataSet.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

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

  virtual vlDataSet *MakeObject() = 0;
  virtual int NumberOfCells() = 0;
  virtual int NumberOfPoints() = 0;
  virtual float *GetPoint(int ptId) = 0;
  virtual vlCell *GetCell(int cellId) = 0;
  virtual vlMapper *MakeMapper() = 0;
  virtual void Initialize();
  virtual void Update() {};

  unsigned long int GetMTime();

  virtual void ComputeBounds();
  float *GetBounds();
  float *GetCenter();
  float GetLength();
  
  vlPointData *GetPointData() {return &this->PointData;};

protected:
  vlPointData PointData;   // Scalars, vectors, etc. associated w/ each point
  vlTimeStamp ComputeTime; // Time at which bounds, center, etc. computed
  float Bounds[6];
  vlMapper *Mapper;
};

#endif


