/*=========================================================================

  Program:   Visualization Toolkit
  Module:    IPoints.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkIntPoints - integer representation of 3D points
// .SECTION Description
// vtkIntPoints is a concrete implementation of vtkPoints. Points are 
// represented using integer values.

#ifndef __vtkIntPoints_h
#define __vtkIntPoints_h

#include "Points.hh"
#include "IntArray.hh"

class vtkIntPoints : public vtkPoints
{
public:
  vtkIntPoints() {};
  vtkIntPoints(const vtkIntPoints& fp) {this->P = fp.P;};
  vtkIntPoints(const int sz, const int ext=1000):P(3*sz,3*ext){};
  ~vtkIntPoints() {};
  int Allocate(const int sz, const int ext=1000) {return this->P.Allocate(3*sz,3*ext);};
  void Initialize() {this->P.Initialize();};
  char *GetClassName() {return "vtkIntPoints";};

  // vtkPoint interface
  vtkPoints *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "int";};
  int GetNumberOfPoints() {return (P.GetMaxId()+1)/3;};
  void Squeeze() {this->P.Squeeze();};
  float *GetPoint(int i);
  void GetPoint(int id, float x[3]);
  void SetPoint(int i, int x[3]);
  void SetPoint(int i, float x[3]);
  void InsertPoint(int i, int *x);
  void InsertPoint(int i, float *x);
  int InsertNextPoint(int *x);
  int InsertNextPoint(float *x);
  void GetPoints(vtkIdList& ptId, vtkFloatPoints& fp);

  // miscellaneous
  int *GetPtr(const int id);
  int *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkIntPoints &operator=(const vtkIntPoints& fp);
  void operator+=(const vtkIntPoints& fp) {this->P += fp.P;};
  void Reset() {this->P.Reset();};

protected:
  vtkIntArray P;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline int *vtkIntPoints::GetPtr(const int id)
{
  return this->P.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of points to 
// write. Use the method WrotePtr() to mark completion of write.
inline int *vtkIntPoints::WritePtr(const int id, const int number)
{
  return this->P.WritePtr(id,3*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkIntPoints::WrotePtr() {}

inline void vtkIntPoints::GetPoint(int id, float x[3])
{
  int *p=this->P.GetPtr(3*id); 
  x[0] = (float)p[0]; x[1] = (float)p[1]; x[2] = (float)p[2];
}

inline void vtkIntPoints::SetPoint(int i, float x[3]) 
{
  i*=3; 
  this->P[i]=(int)x[0]; 
  this->P[i+1]=(int)x[1]; 
  this->P[i+2]=(int)x[2];
}

inline void vtkIntPoints::SetPoint(int i, int x[3]) 
{
  i *= 3; 
  this->P[i] = x[0]; 
  this->P[i+1] = x[1]; 
  this->P[i+2] = x[2];
}

inline void vtkIntPoints::InsertPoint(int i, int *x) 
{
  this->P.InsertValue(3*i+2, x[2]);
  this->P[3*i] =  x[0];
  this->P[3*i+1] = x[1];
}

inline void vtkIntPoints::InsertPoint(int i, float *x) 
{
  this->P.InsertValue(3*i+2, (int)x[2]);
  this->P[3*i] = (int)x[0];
  this->P[3*i+1] = (int)x[1];
}

inline int vtkIntPoints::InsertNextPoint(int *x) 
{
  int id = this->P.GetMaxId() + 3;
  this->P.InsertValue(id,x[2]);
  this->P[id-2] = x[0];
  this->P[id-1] = x[1];
  return id/3;
}

inline int vtkIntPoints::InsertNextPoint(float *x) 
{
  int id = this->P.GetMaxId() + 3;
  this->P.InsertValue(id,(int)x[2]);
  this->P[id-2] = (int)x[0];
  this->P[id-1] = (int)x[1];
  return id/3;
}

#endif
