/*=========================================================================

  Program:   Visualization Library
  Module:    Vectors.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract interface to 3D vectors.
//
#ifndef __vlVectors_h
#define __vlVectors_h

#include "Object.hh"

class vlIdList;
class vlFloatVectors;

class vlVectors : public vlObject 
{
public:
 vlVectors();
  virtual ~vlVectors() {};
  virtual vlVectors *MakeObject(int sze, int ext=1000) = 0;
  virtual int NumberOfVectors() = 0;
  virtual float *GetVector(int i) = 0;
  virtual void SetVector(int i,float x[3]) = 0;       // fast insert
  virtual void InsertVector(int i, float x[3]) = 0;   // allocates memory as necessary
  void GetVectors(vlIdList& ptId, vlFloatVectors& fp);
  char *GetClassName() {return "vlVectors";};
  void PrintSelf(ostream& os, vlIndent indent);
  virtual void ComputeMaxNorm();
  float GetMaxNorm();

protected:
  float MaxNorm;
  vlTimeStamp ComputeTime; // Time at which range computed
};

#endif
