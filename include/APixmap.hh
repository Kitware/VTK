/*=========================================================================

  Program:   Visualization Toolkit
  Module:    APixmap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkAPixmap - scalar data in rgba (color + transparency) form
// .SECTION Description
// vtkAPixmap is a concrete implementation of vtkScalars. Scalars are
// represented using three values for color (red, green, blue) plus alpha
// transparency value. Each of r,g,b,a components ranges from (0,255) (i.e.,
// an unsigned char value).

#ifndef __vtkAPixmap_h
#define __vtkAPixmap_h

#include "CoScalar.hh"
#include "UCArray.hh"

class vtkAPixmap : public vtkColorScalars 
{
public:
  vtkAPixmap() {};
  ~vtkAPixmap() {};
  vtkAPixmap(const vtkAPixmap& fs) {this->S = fs.S;};
  vtkAPixmap(const int sz, const int ext=1000):S(4*sz,4*ext){};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(4*sz,4*ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vtkAPixmap";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1)/4;};
  void Squeeze() {this->S.Squeeze();};
  int GetNumberOfValuesPerScalar() {return 4;};

  // miscellaneous
  vtkAPixmap &operator=(const vtkAPixmap& fs);
  void operator+=(const vtkAPixmap& fs) {this->S += fs.S;};
  void Reset() {this->S.Reset();};
  unsigned char *GetPtr(const int id);
  unsigned char *WritePtr(const int id, const int number);
  void WrotePtr();

  // vtkColorScalar interface.
  unsigned char *GetColor(int id);
  void GetColor(int id, unsigned char rgba[4]);
  void SetColor(int id, unsigned char rgba[4]);
  void InsertColor(int id, unsigned char rgba[4]);
  int InsertNextColor(unsigned char rgba[4]);

protected:
  vtkUnsignedCharArray S;
};

// Description:
// Return a rgba color at array location i.
inline unsigned char *vtkAPixmap::GetColor(int i) 
{
  return this->S.GetPtr(4*i);
}

// Description:
// Get pointer to array of data starting at data position "id".
inline unsigned char *vtkAPixmap::GetPtr(const int id)
{
  return this->S.GetPtr(4*id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline unsigned char *vtkAPixmap::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(4*id,4*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkAPixmap::WrotePtr() {}

#endif
