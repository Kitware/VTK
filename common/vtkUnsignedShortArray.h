/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedShortArray.h
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
// .NAME vtkUnsignedShortArray - dynamic, self-adjusting unsigned short integer array
// .SECTION Description
// vtkUnsignedShortArray is an array of unsigned short integer numbers. It provides methods
// for insertion and retrieval of integer values, and will 
// automatically resize itself to hold new data.

#ifndef __vtkUnsignedShortArray_h
#define __vtkUnsignedShortArray_h

#include "vtkReferenceCount.h"

class VTK_EXPORT vtkUnsignedShortArray : public vtkReferenceCount 
{
public:
  vtkUnsignedShortArray():Array(NULL),Size(0),MaxId(-1),Extend(1000) {};
  int Allocate(const int sz, const int ext=1000);
  void Initialize();
  vtkUnsignedShortArray(const int sz, const int ext=1000);
  vtkUnsignedShortArray(const vtkUnsignedShortArray& ia);
  ~vtkUnsignedShortArray();
  static vtkUnsignedShortArray *New() {return new vtkUnsignedShortArray;};
  const char *GetClassName() {return "vtkUnsignedShortArray";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // access/insertion methods
  unsigned short GetValue(const int id);
  void SetNumberOfValues(const int number);
  void SetValue(const int id, const unsigned short value);
  vtkUnsignedShortArray &InsertValue(const int id, const unsigned short i);
  int InsertNextValue(const unsigned short);
  unsigned short *GetPointer(const int id);
  unsigned short *WritePointer(const int id, const int number);

  // special operators
  vtkUnsignedShortArray &operator=(const vtkUnsignedShortArray& ia);
  void operator+=(const vtkUnsignedShortArray& ia);
  void operator+=(const unsigned short i);

  // miscellaneous methods
  void Squeeze();
  int GetSize();
  int GetMaxId();
  unsigned short *GetArray();
  void Reset();

private:
  unsigned short *Array;   // pointer to data
  int Size;       // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;     // grow array by this point
  unsigned short *Resize(const int sz);  // function to resize data
};

// Description:
// Get the data at a particular index.
inline unsigned short vtkUnsignedShortArray::GetValue(const int id) {return this->Array[id];};

// Description:
// Specify the number of values for this object to hold. Does an
// allocation as well as setting the MaxId ivar. Used in conjunction with
// SetValue() method for fast insertion.
inline void vtkUnsignedShortArray::SetNumberOfValues(const int number) 
{
  this->Allocate(number);
  this->MaxId = number - 1;
}

// Description:
// Set the data at a particular index. Does not do range checking. Make sure
// you use the method SetNumberOfValues() before inserting data.
inline void vtkUnsignedShortArray::SetValue(const int id, const unsigned short value) 
{
  this->Array[id] = value;
}

// Description:
// Get the address of a particular data index.
inline unsigned short *vtkUnsignedShortArray::GetPointer(const int id) {return this->Array + id;};

// Description:
// Get the address of a particular data index. Make sure data is allocated
// for the number of items requested. Set MaxId according to the number of
// data values requested.
inline unsigned short *vtkUnsignedShortArray::WritePointer(const int id, const int number) 
{
  int newSize=id+number;
  if ( newSize > this->Size ) this->Resize(newSize);
  if ( (--newSize) > this->MaxId ) this->MaxId = newSize;
  return this->Array + id;
}

// Description:
// Insert data at a specified position in the array.
inline vtkUnsignedShortArray& vtkUnsignedShortArray::InsertValue(const int id, const unsigned short i)
{
  if ( id >= this->Size ) this->Resize(id+1);
  this->Array[id] = i;
  if ( id > this->MaxId ) this->MaxId = id;
  return *this;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vtkUnsignedShortArray::InsertNextValue(const unsigned short i)
{
  this->InsertValue (++this->MaxId,i); 
  return this->MaxId;
}
inline void vtkUnsignedShortArray::operator+=(const unsigned short i) 
{
  this->InsertNextValue(i);
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vtkUnsignedShortArray::Squeeze() {this->Resize (this->MaxId+1);};

// Description:
// Get the allocated size of the object in terms of number of data items.
inline int vtkUnsignedShortArray::GetSize() {return this->Size;};

// Description:
// Returning the maximum index of data inserted so far.
inline int vtkUnsignedShortArray::GetMaxId() {return this->MaxId;};

// Description:
// Get the pointer to the array. Useful for interfacing to C or 
// FORTRAN routines.
inline unsigned short *vtkUnsignedShortArray::GetArray() {return this->Array;};

// Description:
// Reuse the memory allocated by this object. Objects appears like
// no data has been previously inserted.
inline void vtkUnsignedShortArray::Reset() {this->MaxId = -1;};

#endif




