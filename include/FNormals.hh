/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FNormals.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkFloatNormals - floating point representation of 3D normals
// .SECTION Description
// vtkFloatNormals is a concrete implementation of vtkNormals. Normals are
// represented using float values.

#ifndef __vtkFloatNormals_h
#define __vtkFloatNormals_h

#include "Normals.hh"
#include "FArray.hh"

class vtkFloatNormals : public vtkNormals
{
public:
  vtkFloatNormals() {};
  vtkFloatNormals(const vtkFloatNormals& fn) {this->N = fn.N;};
  vtkFloatNormals(const int sz, const int ext=1000):N(3*sz,3*ext){};
  ~vtkFloatNormals() {};
  int Allocate(const int sz, const int ext=1000) {return this->N.Allocate(3*sz,3*ext);};
  void Initialize() {this->N.Initialize();};
  char *GetClassName() {return "vtkFloatNormals";};

  // vtkNormal interface
  vtkNormals *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfNormals() {return (N.GetMaxId()+1)/3;};
  void Squeeze() {this->N.Squeeze();};
  float *GetNormal(int i) {return this->N.GetPtr(3*i);};
  void GetNormal(int i,float n[3]) {this->vtkNormals::GetNormal(i,n);};
  void SetNormal(int i, float n[3]);
  void InsertNormal(int i, float *n);
  int InsertNextNormal(float *n);

  // miscellaneous
  float *GetPtr(const int id);
  float *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkFloatNormals &operator=(const vtkFloatNormals& fn);
  void operator+=(const vtkFloatNormals& fn);
  void Reset() {this->N.Reset();};

protected:
  vtkFloatArray N;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatNormals::GetPtr(const int id)
{
  return this->N.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of normals to 
// write. Use the method WrotePtr() to mark completion of write.
inline float *vtkFloatNormals::WritePtr(const int id, const int number)
{
  return this->N.WritePtr(id,3*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkFloatNormals::WrotePtr() {}


inline void vtkFloatNormals::SetNormal(int i, float n[3]) 
{
  i*=3; 
  this->N[i]=n[0]; 
  this->N[i+1]=n[1]; 
  this->N[i+2]=n[2];
}

inline void vtkFloatNormals::InsertNormal(int i, float *n) 
{
  this->N.InsertValue(3*i+2, n[2]);
  this->N[3*i] =  n[0];
  this->N[3*i+1] =  n[1];
}

inline int vtkFloatNormals::InsertNextNormal(float *n) 
{
  int id = this->N.GetMaxId() + 3;
  this->N.InsertValue(id,n[2]);
  this->N[id-2] = n[0];
  this->N[id-1] = n[1];
  return id/3;
}

#endif
