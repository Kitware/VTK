/*=========================================================================

  Program:   Visualization Library
  Module:    Bitmap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlBitmap - scalar data in bitmap form
// .SECTION Description
// vlBitmap is a concrete implementation of vlScalars. Scalars are
// represented using a packed character array of (0,1) values.

#ifndef __vlBitmap_h
#define __vlBitmap_h

#include "CoScalar.hh"
#include "BArray.hh"

class vlBitmap : public vlColorScalars 
{
public:
  vlBitmap() {};
  ~vlBitmap() {};
  vlBitmap(const vlBitmap& fs) {this->S = fs.S;};
  vlBitmap(const int sz, const int ext=1000):S(sz,ext){};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vlBitmap";};

  // vlScalar interface
  vlScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfValuesPerPoint() {return 1;};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};

  // miscellaneous
  vlBitmap &operator=(const vlBitmap& fs);
  void operator+=(const vlBitmap& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};

  // vlColorScalar interface.
  unsigned char *GetColor(int id);
  void GetColor(int id, unsigned char rgb[3]);
  void SetColor(int id, unsigned char b[1]);
  void InsertColor(int id, unsigned char b[1]);
  int InsertNextColor(unsigned char b[1]);

protected:
  vlBitArray S;
};

#endif
