/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Bitmap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkBitmap - scalar data in bitmap form
// .SECTION Description
// vtkBitmap is a concrete implementation of vtkScalars. Scalars are
// represented using a packed character array of (0,1) values.

#ifndef __vtkBitmap_h
#define __vtkBitmap_h

#include "CoScalar.hh"
#include "BArray.hh"

class vtkBitmap : public vtkColorScalars 
{
public:
  vtkBitmap() {};
  ~vtkBitmap() {};
  vtkBitmap(const vtkBitmap& fs) {this->S = fs.S;};
  vtkBitmap(const int sz, const int ext=1000):S(sz,ext){};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vtkBitmap";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};

  // miscellaneous
  vtkBitmap &operator=(const vtkBitmap& fs);
  void operator+=(const vtkBitmap& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);

  // vtkColorScalar interface.
  unsigned char *GetColor(int id);
  void GetColor(int id, unsigned char rgba[4]);
  void SetColor(int id, unsigned char rgba[4]);
  void InsertColor(int id, unsigned char rgba[4]);
  int InsertNextColor(unsigned char rgba[4]);

protected:
  vtkBitArray S;
};

// Description:
// Get pointer to byte containing bit in question. You will have to decompose
// byte to obtain appropriate bit value.
inline unsigned char *vtkBitmap::GetPtr(const int id)
{
  return this->S.GetPtr(id);
}

// Description:
// Get pointer to data. Useful for direct writes into object. MaxId is bumped
// by number (and memory allocated if necessary). Id is the locaation you 
// wish to write into; number is the number of rgba colors to write.
inline unsigned char *vtkBitmap::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(id,number);
}

#endif
