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
//
// Floating point representation of 3D vectors.
//
//  use internal floating point array to represent data
//
#ifndef __vlFloatVectors_h
#define __vlFloatVectors_h

#include "Vectors.hh"
#include "FArray.hh"

class vlFloatVectors : public vlVectors
{
public:
  vlFloatVectors() {};
  vlVectors *MakeObject(int sze, int ext=1000);
  int Initialize(const int sz, const int ext=1000) 
    {return this->V.Initialize(3*sz,3*ext);};
  vlFloatVectors(const vlFloatVectors& fv) {this->V = fv.V;};
  vlFloatVectors(const int sz, const int ext=1000):V(3*sz,3*ext){};
  ~vlFloatVectors() {};
  char *GetClassName() {return "vlFloatVectors";};
  int NumVectors() {return (V.GetMaxId()+1)/3;};
  void Reset() {this->V.Reset();};
  vlFloatVectors &operator=(const vlFloatVectors& fv);
  void operator+=(const vlFloatVectors& fv){this->V += fv.V;};

  float *GetVector(int i) {return this->V.GetPtr(3*i);};
  void SetVector(int i, float x[3]) 
    {i*=3; this->V[i]=x[0]; this->V[i+1]=x[1]; this->V[i+2]=x[2];};
  void InsertVector(int i, float *x) {
      this->V.InsertValue(3*i+2, x[2]);
      this->V[3*i] =  x[0];
      this->V[3*i+1] =  x[1];
  }
  int InsertNextVector(float *x) {
    int id = this->V.GetMaxId() + 3;
    this->V.InsertValue(id,x[2]);
    this->V[id-2] = x[0];
    this->V[id-1] = x[1];
    return id/3;
  }

private:
  vlFloatArray V;
};

#endif
