/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDoubleArray.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkDoubleArray - dynamic, self-adjusting double precision array
// .SECTION Description
// vtkDoubleArray is an array of double precision numbers. It provides methods
// for insertion and retrieval of double precision values, and will 
// automatically resize itself to hold new data.

#ifndef __vtkDoubleArray_h
#define __vtkDoubleArray_h

#include "vtkObject.hh"

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
// Reuse the memory allocated by this object. Objects appear as if
// no data has been previously inserted.
inline void vtkDoubleArray::Reset() {this->MaxId = -1;};

#endif
