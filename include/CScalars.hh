/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CScalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCharScalars - unsigned char representation of scalar data
// .SECTION Description
// vtkCharScalars is a concrete implementation of vtkScalars. Scalars are
// represented using char values.

#ifndef __vtkCharScalars_h
#define __vtkCharScalars_h

#include "Scalars.hh"
#include "CArray.hh"

class vtkCharScalars : public vtkScalars 
{
public:
  vtkCharScalars() {};
  vtkCharScalars(const vtkCharScalars& cs) {this->S = cs.S;};
  vtkCharScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vtkCharScalars() {};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vtkCharScalars";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "char";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};
  float GetScalar(int i) {return (float)this->S[i];};
  void SetScalar(int i, unsigned char s) {this->S[i] = s;};
  void SetScalar(int i, float s) {this->S[i] = (char)s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,(char)s);};
  void InsertScalar(int i, unsigned char s) {S.InsertValue(i,s);};
  int InsertNextScalar(unsigned char s) {return S.InsertNextValue(s);};
  int InsertNextScalar(float s) {return S.InsertNextValue((char)s);};
  void GetScalars(vtkIdList& ptIds, vtkFloatScalars& fs);

  // miscellaneous
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkCharScalars &operator=(const vtkCharScalars& cs);
  void operator+=(const vtkCharScalars& cs) {this->S += cs.S;};
  void Reset() {this->S.Reset();};

protected:
  vtkCharArray S;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline unsigned char *vtkCharScalars::GetPtr(const int id)
{
  return this->S.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline unsigned char *vtkCharScalars::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(id,number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkCharScalars::WrotePtr() {}

#endif
