/*=========================================================================

  Program:   Visualization Library
  Module:    AGraymap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlAGraymap - scalar data in intensity + alpha (grayscale  + texture) form
// .SECTION Description
// vlAGraymap is a concrete implementation of vlScalars. vlAGraymap 
// represents scalars using using one value for intensity (grayscale) and
// one value for alpha (transparency). The intensity and alpha values range 
// between (0,255) (i.e., an unsigned char value).

#ifndef __vlAGraymap_h
#define __vlAGraymap_h

#include "CoScalar.hh"
#include "CArray.hh"

class vlAGraymap : public vlColorScalars 
{
public:
  vlAGraymap() {};
  ~vlAGraymap() {};
  vlAGraymap(const vlAGraymap& fs) {this->S = fs.S;};
  vlAGraymap(const int sz, const int ext=1000):S(2*sz,2*ext){};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(2*sz,2*ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vlAGraymap";};

  // vlScalar interface
  vlScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1)/2;};
  void Squeeze() {this->S.Squeeze();};
  int GetNumberOfValuesPerPoint() {return 2;};

  // miscellaneous
  vlAGraymap &operator=(const vlAGraymap& fs);
  void operator+=(const vlAGraymap& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);


  // vlColorScalar interface.
  unsigned char *GetColor(int id);
  void GetColor(int id, unsigned char rgb[3]);
  void SetColor(int id, unsigned char ga[2]);
  void InsertColor(int id, unsigned char ga[2]);
  int InsertNextColor(unsigned char ga[2]);

protected:
  vlCharArray S;
};

// Description:
// Set a intensity/alpha value at a particular array location. Does not do 
// range checking.
inline void vlAGraymap::SetColor(int i, unsigned char ga[2]) 
{
  i *= 2; 
  this->S[i] = ga[0]; 
  this->S[i+1] = ga[1]; 
}

// Description:
// Insert a intensity/alpha value at a particular array location. Does range 
// checking and will allocate additional memory if necessary.
inline void vlAGraymap::InsertColor(int i, unsigned char *ga) 
{
  this->S.InsertValue(2*i+1, ga[1]);
  this->S[2*i] = ga[0];
}

// Description:
// Insert a intensity/alpha value at the next available slot in the array. Will
// allocate memory if necessary.
inline int vlAGraymap::InsertNextColor(unsigned char *ga) 
{
  int id = this->S.GetMaxId() + 1;
  this->S.InsertValue(id,ga[1]);
  this->S[id-1] = ga[0];

  return id/2;
}

// Description:
// Get pointer to ga data at location "id" in the array. Meant for reading 
// data. 
inline unsigned char *vlAGraymap::GetPtr(const int id)
{
  return this->S.GetPtr(2*id);
}

// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary). Id is the locaation you 
// wish to write into; number is the number of ga colors to write.
inline unsigned char *vlAGraymap::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(2*id,2*number);
}


#endif
