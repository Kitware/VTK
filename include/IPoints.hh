/*=========================================================================

  Program:   Visualization Library
  Module:    IPoints.hh
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
// .NAME vlIntPoints - integer representation of 3D points
// .SECTION Description
// vlIntPoints is a concrete implementation of vlPoints. Points are 
// represented using integer values.

#ifndef __vlIntPoints_h
#define __vlIntPoints_h

#include "Points.hh"
#include "IntArray.hh"

class vlIntPoints : public vlPoints
{
public:
  vlIntPoints() {};
  vlIntPoints(const vlIntPoints& fp) {this->P = fp.P;};
  vlIntPoints(const int sz, const int ext=1000):P(3*sz,3*ext){};
  ~vlIntPoints() {};
  int Allocate(const int sz, const int ext=1000) {return this->P.Allocate(3*sz,3*ext);};
  void Initialize() {return this->P.Initialize();};
  char *GetClassName() {return "vlIntPoints";};

  // vlPoint interface
  vlPoints *MakeObject(int sze, int ext=1000);
  int GetNumberOfPoints() {return (P.GetMaxId()+1)/3;};
  void Squeeze() {this->P.Squeeze();};
  float *GetPoint(int i);
  void SetPoint(int i, float x[3]);
  void InsertPoint(int i, float *x);
  int InsertNextPoint(int *x);
  int InsertNextPoint(float *x);

  // miscellaneous
  vlIntPoints &operator=(const vlIntPoints& fp);
  void operator+=(const vlIntPoints& fp) {this->P += fp.P;};
  void Reset() {this->P.Reset();};

protected:
  vlIntArray P;
};

inline void vlIntPoints::SetPoint(int i, float x[3]) 
{
  i*=3; 
  this->P[i]=(int)x[0]; 
  this->P[i+1]=(int)x[1]; 
  this->P[i+2]=(int)x[2];
}

inline void vlIntPoints::InsertPoint(int i, float *x) 
{
  this->P.InsertValue(3*i+2, (int)x[2]);
  this->P[3*i] =  (int)x[0];
  this->P[3*i+1] =  (int)x[1];
}

inline int vlIntPoints::InsertNextPoint(int *x) 
{
  int id = this->P.GetMaxId() + 3;
  this->P.InsertValue(id,x[2]);
  this->P[id-2] = x[0];
  this->P[id-1] = x[1];
  return id/3;
}

inline int vlIntPoints::InsertNextPoint(float *x) 
{
  int id = this->P.GetMaxId() + 3;
  this->P.InsertValue(id,(int)x[2]);
  this->P[id-2] = (int)x[0];
  this->P[id-1] = (int)x[1];
  return id/3;
}

#endif
