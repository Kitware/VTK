/*=========================================================================

  Program:   Visualization Library
  Module:    Pixmap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPixmap - scalar data in RGB (color) form
// .SECTION Description
// vlPixmap is a concrete implementation of vlScalars. Scalars are
// represented using three values for color (red, green, blue). Each of
// r,g,b ranges from (0,255) (i.e., an unsigned char value).

#ifndef __vlPixmap_h
#define __vlPixmap_h

#include "CoScalar.hh"
#include "CArray.hh"

class vlPixmap : public vlColorScalars 
{
public:
  vlPixmap() {};
  ~vlPixmap() {};
  vlPixmap(const vlPixmap& fs) {this->S = fs.S;};
  vlPixmap(const int sz, const int ext=1000):S(3*sz,3*ext){};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(3*sz,3*ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vlPixmap";};

  // vlScalar interface
  vlScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1)/3;};
  void Squeeze() {this->S.Squeeze();};
  int GetNumberOfValuesPerPoint() {return 3;};

  // miscellaneous
  vlPixmap &operator=(const vlPixmap& fs);
  void operator+=(const vlPixmap& fs) {this->S += fs.S;};
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
// Set a rgba color value at a particular array location. Does not do range 
// checking.
inline void vlPixmap::SetColor(int i, unsigned char rgba[4]) 
{
  i *= 3; 
  this->S[i] = rgba[0]; 
  this->S[i+1] = rgba[1]; 
  this->S[i+2] = rgba[2];
}

// Description:
// Insert a rgba color value at a particular array location. Does range 
// checking and will allocate additional memory if necessary.
inline void vlPixmap::InsertColor(int i, unsigned char rgba[4]) 
{
  this->S.InsertValue(3*i+2, rgba[2]);
  this->S[3*i] = rgba[0];
  this->S[3*i+1] = rgba[1];
}

// Description:
// Insert a rgba value at the next available slot in the array. Will allocate
// memory if necessary.
inline int vlPixmap::InsertNextColor(unsigned char *rgba) 
{
  int id = this->S.GetMaxId() + 3;
  this->S.InsertValue(id,rgba[2]);
  this->S[id-2] = rgba[0];
  this->S[id-1] = rgba[1];
  return id/3;
}

// Description:
// Get pointer to rgba data at location "id" in the array. Meant for reading 
// data. 
inline unsigned char *vlPixmap::GetPtr(const int id)
{
  return this->S.GetPtr(3*id);
}

// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary). Id is the locaation you 
// wish to write into; number is the number of rgba triplets to write.
inline unsigned char *vlPixmap::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(3*id,3*number);
}

#endif
