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
  unsigned char *GetUCharPtr() {return S.GetPtr(0);};

  // miscellaneous
  vlAGraymap &operator=(const vlAGraymap& fs);
  void operator+=(const vlAGraymap& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};
  unsigned char *WriteInto(int id, int number);

  // vlColorScalar interface.
  unsigned char *GetColor(int id);
  void GetColor(int id, unsigned char rgb[3]);
  void SetColor(int id, unsigned char rgb[3]);
  void InsertColor(int id, unsigned char rgb[3]);
  int InsertNextColor(unsigned char rgb[3]);

protected:
  vlCharArray S;
};

// Description:
// Return a rgb triplet at array location i.
inline unsigned char *vlAGraymap::GetColor(int i) {return this->S.GetPtr(2*i);};

// Description:
// Set a rgb value at a particular array location. Does not do range 
// checking.
inline void vlAGraymap::SetColor(int i, unsigned char rgb[3]) 
{
  i *= 2; 
  this->S[i] = rgb[0]; 
  this->S[i+1] = rgb[1]; 
  this->S[i+2] = rgb[2];
}

// Description:
// Insert a rgb value at a particular array location. Does range checking
// and will allocate additional memory if necessary.
inline void vlAGraymap::InsertColor(int i, unsigned char *rgb) 
{
  this->S.InsertValue(2*i+2, rgb[2]);
  this->S[2*i] = rgb[0];
  this->S[2*i+1] = rgb[1];
}

// Description:
// Insert a rgb value at the next available slot in the array. Will allocate
// memory if necessary.
inline int vlAGraymap::InsertNextColor(unsigned char *rgb) 
{
  int id = this->S.GetMaxId() + 2;
  this->S.InsertValue(id,rgb[2]);
  this->S[id-2] = rgb[0];
  this->S[id-1] = rgb[1];
  return id/2;
}

// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary).
unsigned char *vlAGraymap::WriteInto(int id, int number)
{
  return this->S.WriteInto(id, number);
}

#endif
