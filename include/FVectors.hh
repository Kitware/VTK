/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FVectors.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkFloatVectors - floating point representation of 3D vectors
// .SECTION Description
// vtkFloatVectors is a concrete implementation of vtkVectors. Vectors are
// represented using float values.

#ifndef __vtkFloatVectors_h
#define __vtkFloatVectors_h

#include "Vectors.hh"
#include "FArray.hh"

class vtkFloatVectors : public vtkVectors
{
public:
  vtkFloatVectors() {};
  vtkFloatVectors(const vtkFloatVectors& fv) {this->V = fv.V;};
  vtkFloatVectors(const int sz, const int ext=1000):V(3*sz,3*ext){};
  ~vtkFloatVectors() {};
  int Allocate(const int sz, const int ext=1000) {return this->V.Allocate(3*sz,3*ext);};
  void Initialize() {this->V.Initialize();};
  char *GetClassName() {return "vtkFloatVectors";};

  // vtkVector interface
  vtkVectors *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfVectors() {return (V.GetMaxId()+1)/3;};
  void Squeeze() {this->V.Squeeze();};
  float *GetVector(int i) {return this->V.GetPtr(3*i);};
  void GetVector(int i,float v[3]) {this->vtkVectors::GetVector(i,v);};
  void SetVector(int i, float v[3]);
  void InsertVector(int i, float *v);
  int InsertNextVector(float *v);

  // miscellaneous
  float *GetPtr(const int id);
  float *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkFloatVectors &operator=(const vtkFloatVectors& fv);
  void operator+=(const vtkFloatVectors& fv){this->V += fv.V;};
  void Reset() {this->V.Reset();};

protected:
  vtkFloatArray V;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatVectors::GetPtr(const int id)
{
  return this->V.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of vectors to 
// write. Use the method WrotePtr() to mark completion of write.
inline float *vtkFloatVectors::WritePtr(const int id, const int number)
{
  return this->V.WritePtr(id,3*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkFloatVectors::WrotePtr() {}


inline void vtkFloatVectors::SetVector(int i, float v[3]) 
{
  i*=3; 
  this->V[i]=v[0]; 
  this->V[i+1]=v[1]; 
  this->V[i+2]=v[2];
}

inline void vtkFloatVectors::InsertVector(int i, float *v) 
{
  this->V.InsertValue(3*i+2, v[2]);
  this->V[3*i] =  v[0];
  this->V[3*i+1] =  v[1];
}

inline int vtkFloatVectors::InsertNextVector(float *v) 
{
  int id = this->V.GetMaxId() + 3;
  this->V.InsertValue(id,v[2]);
  this->V[id-2] = v[0];
  this->V[id-1] = v[1];
  return id/3;
}

#endif
