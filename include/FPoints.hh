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
//
// Floating point representation of 3D points.
//
//  use internal floating point array to represent data
//
#ifndef __vlFloatPoints_h
#define __vlFloatPoints_h

#include "Points.hh"
#include "FArray.hh"

class vlFloatPoints : public vlPoints
{
public:
  vlFloatPoints() {};
  vlPoints *MakeObject(int sze, int ext=1000);
  int Initialize(const int sz, const int ext=1000) 
    {return this->P.Initialize(3*sz,3*ext);};
  vlFloatPoints(const vlFloatPoints& fp) {this->P = fp.P;};
  vlFloatPoints(const int sz, const int ext=1000):P(3*sz,3*ext){};
  ~vlFloatPoints() {};
  char *GetClassName() {return "vlFloatPoints";};
  int NumPoints() {return (P.GetMaxId()+1)/3;};
  void Reset() {this->P.Reset();};
  vlFloatPoints &operator=(const vlFloatPoints& fp);
  void operator+=(const vlFloatPoints& fp) {this->P += fp.P;};

  float *GetPoint(int i) {return this->P.GetPtr(3*i);};
  void SetPoint(int i, float x[3]) 
    {i*=3; this->P[i]=x[0]; this->P[i+1]=x[1]; this->P[i+2]=x[2];};
  void InsertPoint(int i, float *x) {
    this->P.InsertValue(3*i+2, x[2]);
    this->P[3*i] =  x[0];
    this->P[3*i+1] =  x[1];
  }
  int InsertNextPoint(float *x) {
    int id = this->P.GetMaxId() + 3;
    this->P.InsertValue(id,x[2]);
    this->P[id-2] = x[0];
    this->P[id-1] = x[1];
    return id/3;
  }

private:
  vlFloatArray P;
};

#endif
