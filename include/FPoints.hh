/*=========================================================================

  Program:   Visualization Library
  Module:    FPoints.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlFloatPoints - floating point representation of 3D points
// .SECTION Description
// vlFloatPoints is a concrete implementation of vlPoints. Points are
// represented using float values.

#ifndef __vlFloatPoints_h
#define __vlFloatPoints_h

#include "Points.hh"
#include "FArray.hh"

class vlFloatPoints : public vlPoints
{
public:
  vlFloatPoints() {};
  vlFloatPoints(const vlFloatPoints& fp) {this->P = fp.P;};
  vlFloatPoints(const int sz, const int ext=1000):P(3*sz,3*ext){};
  ~vlFloatPoints() {};
  int Allocate(const int sz, const int ext=1000) {return this->P.Allocate(3*sz,3*ext);};
  void Initialize() {return this->P.Initialize();};
  char *GetClassName() {return "vlFloatPoints";};

  // vlPoint interface
  vlPoints *MakeObject(int sze, int ext=1000);
  int GetNumberOfPoints() {return (P.GetMaxId()+1)/3;};
  void Squeeze() {this->P.Squeeze();};
  float *GetPoint(int i) {return this->P.GetPtr(3*i);};
  void SetPoint(int i, float x[3]);
  void InsertPoint(int i, float *x);
  int InsertNextPoint(float *x);

  // miscellaneous
  vlFloatPoints &operator=(const vlFloatPoints& fp);
  void operator+=(const vlFloatPoints& fp) {this->P += fp.P;};
  void Reset() {this->P.Reset();};

protected:
  vlFloatArray P;
};


inline void vlFloatPoints::SetPoint(int i, float x[3]) 
{
  i*=3; 
  this->P[i]=x[0]; 
  this->P[i+1]=x[1]; 
  this->P[i+2]=x[2];
}

inline void vlFloatPoints::InsertPoint(int i, float *x)
{
  this->P.InsertValue(3*i+2, x[2]); // only do range checking once
  this->P[3*i] =  x[0];
  this->P[3*i+1] =  x[1];
}

inline int vlFloatPoints::InsertNextPoint(float *x)
{
  int id = this->P.GetMaxId() + 3;
  this->P.InsertValue(id,x[2]); // only do range checking once
  this->P[id-2] = x[0];
  this->P[id-1] = x[1];
  return id/3;
}

#endif
