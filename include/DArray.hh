/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDoubleArray - dynamic, self adjusting double precision array
// .SECTION Description
// vtkDoubleArray is an array of double precision numbers. It provides methods
// for insertion and retrieval of double precision values, and will 
// automatically resize itself to hold new data.

#ifndef __vtkDoubleArray_h
#define __vtkDoubleArray_h

#include "Object.hh"

class vtkDoubleArray : public vtkObject 
{
public:
  vtkDoubleArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext=1000);
  void Initialize();
  vtkDoubleArray(const int sz, const int ext=1000);
  vtkDoubleArray(const vtkDoubleArray& fa);
  ~vtkDoubleArray();
  virtual char *GetClassName() {return "vtkDoubleArray";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // access/insertion methods
  double GetValue(const int id);
  vtkDoubleArray &InsertValue(const int id, const double f);
  int InsertNextValue(const double f);
  double *GetPtr(const int id);
  double *WritePtr(const int id, const int number);

  // special operators
  vtkDoubleArray &operator=(const vtkDoubleArray& fa);
  void operator+=(const vtkDoubleArray& fa);
  void operator+=(const double f);
  double& operator[](const int i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  void Reset();

private:
  double *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  double *Resize(const int sz);  // function to resize data
};

// Description:
// Get the data at a particular index.
inline double vtkDoubleArray::GetValue(const int id) {return this->Array[id];};

// Description:
// Get the address of a particular data index.
inline double *vtkDoubleArray::GetPtr(const int id) {return this->Array + id;};

// Description:
// Get the address of a particular data index. Make sure data is allocated
// for the number of items requested. Set MaxId according to the number of
// data values requested.
inline double *vtkDoubleArray::WritePtr(const int id, const int number) 
{
  if ( (id + number) > this->Size ) this->Resize(id+number);
  this->MaxId = id + number - 1;
  return this->Array + id;
}

// Description:
// Insert data at a specified position in the array.
inline vtkDoubleArray& vtkDoubleArray::InsertValue(const int id, const double f)
{
  if ( id >= this->Size ) this->Resize(id);
  this->Array[id] = f;
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vtkDoubleArray::InsertNextValue(const double f)
{
  this->InsertValue (++this->MaxId,f); 
  return this->MaxId;
}
inline void vtkDoubleArray::operator+=(const double f) 
{
  this->InsertNextValue(f);
}

// Description:
// Does insert or get (depending on location on lhs or rhs of statement). Does
// not do automatic resizing - user's responsibility to range check.
inline double& vtkDoubleArray::operator[](const int i)
{
  if (i > this->MaxId) this->MaxId = i; 
  return this->Array[i];
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vtkDoubleArray::Squeeze() {this->Resize (this->MaxId+1);};

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vtkDoubleArray::GetSize() {return this->Size;};

// Description:
// Returning the maximum index of data inserted so far.
inline int vtkDoubleArray::GetMaxId() {return this->MaxId;};

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vtkDoubleArray::Reset() {this->MaxId = -1;};

#endif
