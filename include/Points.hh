/*=========================================================================

  Program:   Visualization Library
  Module:    Points.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract interface to 3D points.
//
#ifndef __vlPoints_h
#define __vlPoints_h

#include "Object.hh"

class vlFloatPoints;
class vlIdList;

class vlPoints : public vlObject 
{
public:
  vlPoints();
  virtual ~vlPoints() {};
  virtual vlPoints *MakeObject(int sze, int ext=1000) = 0;
  virtual int NumberOfPoints() = 0;
  virtual float *GetPoint(int i) = 0;
  virtual void SetPoint(int i,float x[3]) = 0;       // fast insert
  virtual void InsertPoint(int i, float x[3]) = 0;   // allocates memory as necessary
  void GetPoints(vlIdList& ptId, vlFloatPoints& fp);
  char *GetClassName() {return "vlPoints";};
  void PrintSelf(ostream& os, vlIndent indent);
  virtual void ComputeBounds();
  float *GetBounds();

protected:
  float Bounds[6];
  vlTimeStamp ComputeTime; // Time at which range computed

};

#endif
