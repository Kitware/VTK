/*=========================================================================

  Program:   Visualization Library
  Module:    Scalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract interface to scalar values.
//
#ifndef __vlScalars_h
#define __vlScalars_h

#include "Object.hh"

class vlIdList;
class vlFloatScalars;

class vlScalars : public vlObject 
{
public:
  vlScalars();
  virtual ~vlScalars() {};
  virtual vlScalars *MakeObject(int sze, int ext=1000) = 0;
  virtual int GetNumberOfScalars() = 0;
  virtual float GetScalar(int i) = 0;
  virtual void SetScalar(int i, float s) = 0;     // fast insert
  virtual void InsertScalar(int i, float s) = 0;  // allocates memory as necessary
  virtual void Squeeze() = 0;

  void GetScalars(vlIdList& ptId, vlFloatScalars& fs);
  char *GetClassName() {return "vlScalars";};
  void PrintSelf(ostream& os, vlIndent indent);
  virtual void ComputeRange();
  float *GetRange();

protected:
  float Range[2];
  vlTimeStamp ComputeTime; // Time at which range computed
};

#endif
