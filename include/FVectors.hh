/*=========================================================================

  Program:   Visualization Library
  Module:    FVectors.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlFloatVectors - floating point representation of 3D vectors
// .SECTION Description
// vlFloatVectors is a concrete implementation of vlVectors. Vectors are
// represented using float values.

#ifndef __vlFloatVectors_h
#define __vlFloatVectors_h

#include "Vectors.hh"
#include "FArray.hh"

class vlFloatVectors : public vlVectors
{
public:
  vlFloatVectors() {};
  vlFloatVectors(const vlFloatVectors& fv) {this->V = fv.V;};
  vlFloatVectors(const int sz, const int ext=1000):V(3*sz,3*ext){};
  ~vlFloatVectors() {};
  int Allocate(const int sz, const int ext=1000) {return this->V.Allocate(3*sz,3*ext);};
  void Initialize() {this->V.Initialize();};
  char *GetClassName() {return "vlFloatVectors";};

  // vlVector interface
  vlVectors *MakeObject(int sze, int ext=1000);
  int GetNumberOfVectors() {return (V.GetMaxId()+1)/3;};
  void Squeeze() {this->V.Squeeze();};
  float *GetVector(int i) {return this->V.GetPtr(3*i);};
  void SetVector(int i, float v[3]);
  void InsertVector(int i, float *v);
  int InsertNextVector(float *v);

  // miscellaneous
  vlFloatVectors &operator=(const vlFloatVectors& fv);
  void operator+=(const vlFloatVectors& fv){this->V += fv.V;};
  void Reset() {this->V.Reset();};

protected:
  vlFloatArray V;
};

inline void vlFloatVectors::SetVector(int i, float v[3]) 
{
  i*=3; 
  this->V[i]=v[0]; 
  this->V[i+1]=v[1]; 
  this->V[i+2]=v[2];
}

inline void vlFloatVectors::InsertVector(int i, float *v) 
{
  this->V.InsertValue(3*i+2, v[2]);
  this->V[3*i] =  v[0];
  this->V[3*i+1] =  v[1];
}

inline int vlFloatVectors::InsertNextVector(float *v) 
{
  int id = this->V.GetMaxId() + 3;
  this->V.InsertValue(id,v[2]);
  this->V[id-2] = v[0];
  this->V[id-1] = v[1];
  return id/3;
}

#endif
