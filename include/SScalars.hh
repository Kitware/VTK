/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SScalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkShortScalars - short integer representation of scalar data
// .SECTION Description
// vtkShortScalars is a concrete implementation of vtkScalars. Scalars are
// represented using short integer values.

#ifndef __vtkShortScalars_h
#define __vtkShortScalars_h

#include "Scalars.hh"
#include "SArray.hh"

class vtkShortScalars : public vtkScalars 
{
public:
  vtkShortScalars() {};
  vtkShortScalars(const vtkShortScalars& ss) {this->S = ss.S;};
  vtkShortScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vtkShortScalars() {};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vtkShortScalars";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "short";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};
  float GetScalar(int i) {return (float)this->S[i];};
  void SetScalar(int i, short s) {this->S[i] = s;};
  void SetScalar(int i, float s) {this->S[i] = (short)s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,(short)s);};
  void InsertScalar(int i, short s) {S.InsertValue(i,s);};
  int InsertNextScalar(short s) {return S.InsertNextValue(s);};
  int InsertNextScalar(float s) {return S.InsertNextValue((short)s);};
  void GetScalars(vtkIdList& ptIds, vtkFloatScalars& fs);

  // miscellaneous
  short *GetPtr(const int id);
  short *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkShortScalars &operator=(const vtkShortScalars& ss);
  void operator+=(const vtkShortScalars& ss) {this->S += ss.S;};
  void Reset() {this->S.Reset();};

protected:
  vtkShortArray S;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline short *vtkShortScalars::GetPtr(const int id)
{
  return this->S.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline short *vtkShortScalars::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(id,number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkShortScalars::WrotePtr() {}

#endif
