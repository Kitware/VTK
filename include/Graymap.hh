/*=========================================================================

  Program:   Visualization Library
  Module:    Graymap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlGraymap - scalar data in grayscale form
// .SECTION Description
// vlGraymap is a concrete implementation of vlScalars. Scalars are
// represented using a single unsigned char for components of gray. 
// Gray values range from (0,255) with 0 being black.

#ifndef __vlGraymap_h
#define __vlGraymap_h

#include "CoScalar.hh"
#include "CArray.hh"

class vlGraymap : public vlColorScalars 
{
public:
  vlGraymap() {};
  ~vlGraymap() {};
  vlGraymap(const vlGraymap& fs) {this->S = fs.S;};
  vlGraymap(const int sz, const int ext=1000):S(sz,ext){};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vlGraymap";};

  // vlScalar interface
  vlScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfValuesPerPoint() {return 1;};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};

  // miscellaneous
  vlGraymap &operator=(const vlGraymap& fs);
  void operator+=(const vlGraymap& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);

  // vlColorScalar interface.
  unsigned char *GetColor(int id);
  void GetColor(int id, unsigned char rgba[4]);
  void SetColor(int id, unsigned char rgba[4]);
  void InsertColor(int id, unsigned char rgba[4]);
  int InsertNextColor(unsigned char rgba[4]);

protected:
  vlCharArray S;
};

// Description:
// Get pointer to rgb data at location "id" in the array. Meant for reading 
// data. 
inline unsigned char *vlGraymap::GetPtr(const int id)
{
  return this->S.GetPtr(id);
}

// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary). Id is the locaation you 
// wish to write into; number is the number of rgb triplets to write.
inline unsigned char *vlGraymap::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(id,number);
}

#endif
