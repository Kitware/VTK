/*=========================================================================

  Program:   Visualization Library
  Module:    FNormals.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Floating point representation of 3D normals.
//
//  use internal floating point array to represent data
//
#ifndef __vlFloatNormals_h
#define __vlFloatNormals_h

#include "Normals.hh"
#include "FArray.hh"

class vlFloatNormals : public vlNormals
{
public:
  vlFloatNormals() {};
  vlNormals *MakeObject(int sze, int ext=1000);
  int Initialize(const int sz, const int ext=1000) 
    {return this->N.Initialize(3*sz,3*ext);};
  vlFloatNormals(const vlFloatNormals& fn) {this->N = fn.N;};
  vlFloatNormals(const int sz, const int ext=1000):N(3*sz,3*ext){};
  ~vlFloatNormals() {};
  char *GetClassName() {return "vlFloatNormals";};
  int NumberOfNormals() {return (N.GetMaxId()+1)/3;};
  void Reset() {this->N.Reset();};
  vlFloatNormals &operator=(const vlFloatNormals& fn);
  void operator+=(const vlFloatNormals& fn);

  float *GetNormal(int i) {return this->N.GetPtr(3*i);};
  void SetNormal(int i, float x[3]) 
    {i*=3; this->N[i]=x[0]; this->N[i+1]=x[1]; this->N[i+2]=x[2];};
  void InsertNormal(int i, float *x) {
      this->N.InsertValue(3*i+2, x[2]);
      this->N[3*i] =  x[0];
      this->N[3*i+1] =  x[1];
  }
  int InsertNextNormal(float *x) {
    int id = this->N.GetMaxId() + 3;
    this->N.InsertValue(id,x[2]);
    this->N[id-2] = x[0];
    this->N[id-1] = x[1];
    return id/3;
  }

private:
  vlFloatArray N;
};

#endif
