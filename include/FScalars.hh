/*=========================================================================

  Program:   Visualization Toolkit
  Module:    FScalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkFloatScalars - floating point representation of scalar data
// .SECTION Description
// vtkFloatScalars is a concrete implementation of vtkScalars. Scalars are
// represented using float values.

#ifndef __vtkFloatScalars_h
#define __vtkFloatScalars_h

#include "Scalars.hh"
#include "FArray.hh"

class vtkFloatScalars : public vtkScalars 
{
public:
  vtkFloatScalars() {};
  vtkFloatScalars(const vtkFloatScalars& fs) {this->S = fs.S;};
  vtkFloatScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vtkFloatScalars() {};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vtkFloatScalars";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};
  float GetScalar(int i) {return this->S.GetValue(i);};
  void SetScalar(int i, float s) {this->S[i] = s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,s);};
  int InsertNextScalar(float s) {return S.InsertNextValue(s);};
  void GetScalars(vtkIdList& ptIds, vtkFloatScalars& fs);

  // miscellaneous
  float *GetPtr(const int id);
  float *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkFloatScalars &operator=(const vtkFloatScalars& fs);
  void operator+=(const vtkFloatScalars& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};

protected:
  vtkFloatArray S;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatScalars::GetPtr(const int id)
{
  return this->S.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline float *vtkFloatScalars::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(id,number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkFloatScalars::WrotePtr() {}

#endif
