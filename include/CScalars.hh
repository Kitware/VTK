/*=========================================================================

  Program:   Visualization Library
  Module:    CScalars.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlCharScalars - unsigned char representation of scalar data
// .SECTION Description
// vlCharScalars is a concrete implementation of vlScalars. Scalars are
// represented using char values.

#ifndef __vlCharScalars_h
#define __vlCharScalars_h

#include "Scalars.hh"
#include "CArray.hh"

class vlCharScalars : public vlScalars 
{
public:
  vlCharScalars() {};
  vlCharScalars(const vlCharScalars& cs) {this->S = cs.S;};
  vlCharScalars(const int sz, const int ext=1000):S(sz,ext){};
  ~vlCharScalars() {};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vlCharScalars";};

  // vlScalar interface
  vlScalars *MakeObject(int sze, int ext=1000);
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
  void GetScalars(vlIdList& ptIds, vlFloatScalars& fs);

  // miscellaneous
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);
  vlCharScalars &operator=(const vlCharScalars& cs);
  void operator+=(const vlCharScalars& cs) {this->S += cs.S;};
  void Reset() {this->S.Reset();};

protected:
  vlCharArray S;
};

// Description:
// Get pointer to scalar data at location "id" in the array. Meant for reading 
// data. 
inline unsigned char *vlCharScalars::GetPtr(const int id)
{
  return this->S.GetPtr(id);
}

// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary). Id is the locaation you 
// wish to write into; number is the number of scalars to write.
inline unsigned char *vlCharScalars::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(id,number);
}

#endif
