/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FPoints.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkFloatPoints - floating point representation of 3D points
// .SECTION Description
// vtkFloatPoints is a concrete implementation of vtkPoints. Points are
// represented using float values.

#ifndef __vtkFloatPoints_h
#define __vtkFloatPoints_h

#include "Points.hh"
#include "FArray.hh"

class vtkFloatPoints : public vtkPoints
{
public:
  vtkFloatPoints() {};
  vtkFloatPoints(const vtkFloatPoints& fp) {this->P = fp.P;};
  vtkFloatPoints(const int sz, const int ext=1000):P(3*sz,3*ext){};
  ~vtkFloatPoints() {};
  int Allocate(const int sz, const int ext=1000) {return this->P.Allocate(3*sz,3*ext);};
  void Initialize() {this->P.Initialize();};
  char *GetClassName() {return "vtkFloatPoints";};

  // vtkPoint interface
  vtkPoints *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfPoints() {return (P.GetMaxId()+1)/3;};
  void Squeeze() {this->P.Squeeze();};
  float *GetPoint(int id) {return this->P.GetPtr(3*id);};
  void GetPoint(int id, float x[3]);
  void SetPoint(int id, float x[3]);
  void InsertPoint(int id, float *x);
  int InsertNextPoint(float *x);
  void GetPoints(vtkIdList& ptId, vtkFloatPoints& fp);

  // miscellaneous
  float *GetPtr(const int id);
  float *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkFloatPoints &operator=(const vtkFloatPoints& fp);
  void operator+=(const vtkFloatPoints& fp) {this->P += fp.P;};
  void Reset() {this->P.Reset();};

protected:
  vtkFloatArray P;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatPoints::GetPtr(const int id)
{
  return this->P.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline float *vtkFloatPoints::WritePtr(const int id, const int number)
{
  return this->P.WritePtr(id,3*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkFloatPoints::WrotePtr() {}

inline void vtkFloatPoints::GetPoint(int id, float x[3])
{
  float *p=this->P.GetPtr(3*id); 
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}

inline void vtkFloatPoints::SetPoint(int id, float x[3]) 
{
  id*=3; 
  this->P[id]=x[0]; 
  this->P[id+1]=x[1]; 
  this->P[id+2]=x[2];
}

inline void vtkFloatPoints::InsertPoint(int id, float *x)
{
  this->P.InsertValue(3*id+2, x[2]); // only do range checking once
  this->P[3*id] = x[0];
  this->P[3*id+1] =  x[1];
}

inline int vtkFloatPoints::InsertNextPoint(float *x)
{
  int id = this->P.GetMaxId() + 3;
  this->P.InsertValue(id,x[2]); // only do range checking once
  this->P[id-2] = x[0];
  this->P[id-1] = x[1];
  return id/3;
}

#endif
