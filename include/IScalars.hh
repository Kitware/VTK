/*=========================================================================

  Program:   Visualization Toolkit
  Module:    IScalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkIntScalars - integer representation of scalar data
// .SECTION Description
// vtkIntScalars is a concrete implementation of vtkScalars. Scalars are
// represented using integer values.

#ifndef __vtkIntScalars_h
#define __vtkIntScalars_h

#include "Scalars.hh"
#include "SArray.hh"

class vtkIntScalars : public vtkScalars 
{
public:
  vtkIntScalars() {};
  vtkIntScalars(const vtkIntScalars& ss) {this->S = ss.S;};
  vtkIntScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vtkIntScalars() {};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vtkIntScalars";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "int";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};
  float GetScalar(int i) {return (float)this->S[i];};
  void SetScalar(int i, int s) {this->S[i] = s;};
  void SetScalar(int i, float s) {this->S[i] = (int)s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,(int)s);};
  void InsertScalar(int i, int s) {S.InsertValue(i,s);};
  int InsertNextScalar(int s) {return S.InsertNextValue(s);};
  int InsertNextScalar(float s) {return S.InsertNextValue((int)s);};
  void GetScalars(vtkIdList& ptIds, vtkFloatScalars& fs);

  // miscellaneous
  int *GetPtr(const int id);
  int *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkIntScalars &operator=(const vtkIntScalars& is);
  void operator+=(const vtkIntScalars& is) {this->S += is.S;};
  void Reset() {this->S.Reset();};

protected:
  vtkIntArray S;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline int *vtkIntScalars::GetPtr(const int id)
{
  return this->S.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline int *vtkIntScalars::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(id,number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkIntScalars::WrotePtr() {}

#endif
