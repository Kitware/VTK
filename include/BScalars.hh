/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BScalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkBitScalars - packed bit (0/1) representation of scalar data
// .SECTION Description
// vtkBitScalars is a concrete implementation of vtkScalars. Scalars are
// represented using a packed bit array. Only possible scalar values are
// (0/1).

#ifndef __vtkBitScalars_h
#define __vtkBitScalars_h

#include "Scalars.hh"
#include "BArray.hh"

class vtkBitScalars : public vtkScalars 
{
public:
  vtkBitScalars() {};
  vtkBitScalars(const vtkBitScalars& cs) {this->S = cs.S;};
  vtkBitScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vtkBitScalars() {};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vtkBitScalars";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "bit";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};
  float GetScalar(int i) {return (float)this->S.GetValue(i);};
  void SetScalar(int i, int s) {this->S.SetValue(i,s);};
  void SetScalar(int i, float s) {this->S.SetValue(i,(int)s);};
  void InsertScalar(int i, float s) {S.InsertValue(i,(int)s);};
  void InsertScalar(int i, int s) {S.InsertValue(i,s);};
  int InsertNextScalar(int s) {return S.InsertNextValue(s);};
  int InsertNextScalar(float s) {return S.InsertNextValue((int)s);};
  void GetScalars(vtkIdList& ptIds, vtkFloatScalars& fs);

  // miscellaneous
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkBitScalars &operator=(const vtkBitScalars& cs);
  void operator+=(const vtkBitScalars& cs) {this->S += cs.S;};
  void Reset() {this->S.Reset();};

protected:
  vtkBitArray S;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline unsigned char *vtkBitScalars::GetPtr(const int id)
{
  return this->S.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline unsigned char *vtkBitScalars::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(id,number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkBitScalars::WrotePtr() {}

#endif
