/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntArray.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkIntArray - dynamic, self-adjusting integer array
// .SECTION Description
// vtkIntArray is an array of integer numbers. It provides methods
// for insertion and retrieval of integer values, and will 
// automatically resize itself to hold new data.

#ifndef __vtkIntArray_h
#define __vtkIntArray_h

#include "vtkDataArray.h"

class VTK_EXPORT vtkIntArray : public vtkDataArray
{
public:
  vtkIntArray(int numComp=1);
  ~vtkIntArray();
  int Allocate(const int sz, const int ext=1000);
  void Initialize();
  static vtkIntArray *New() {return new vtkIntArray;};
  const char *GetClassName() {return "vtkIntArray";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // satisfy vtkDataArray API
  vtkDataArray *MakeObject() {return new vtkIntArray(this->NumberOfComponents);};
  int GetDataType() {return VTK_INT;};
  void SetNumberOfTuples(const int number);
  float *GetTuple(const int i);
  void GetTuple(const int i, float * tuple);
  void SetTuple(const int i, const float * tuple);
  void InsertTuple(const int i, const float * tuple);
  int InsertNextTuple(const float * tuple);
  void Squeeze();

  // native access/insertion methods
  int GetValue(const int id);
  void SetNumberOfValues(const int number);
  void SetValue(const int id, const int value);
  void InsertValue(const int id, const int i);
  int InsertNextValue(const int i);
  int *GetPointer(const int id) {return this->Array + id;}
  int *WritePointer(const int id, const int number);
  void *GetVoidPointer(const int id) {return (void *)this->GetPointer(id);};
  void DeepCopy(vtkDataArray& ia);

  void SetArray(int* array, int size, int save);

private:
  int *Array;   // pointer to data
  int *Resize(const int sz);  // function to resize data

  int TupleSize; //used for data conversion
  float *Tuple;

  int SaveUserArray;
};

// Description:
// Get the data at a particular index.
inline int vtkIntArray::GetValue(const int id) {return this->Array[id];}

// Description:
// Specify the number of values for this object to hold. Does an
// allocation as well as setting the MaxId ivar. Used in conjunction with
// SetValue() method for fast insertion.
inline void vtkIntArray::SetNumberOfValues(const int number) 
{
  this->Allocate(number);
  this->MaxId = number - 1;
}

// Description:
// Set the data at a particular index. Does not do range checking. Make sure
// you use the method SetNumberOfValues() before inserting data.
inline void vtkIntArray::SetValue(const int id, const int value) 
{
  this->Array[id] = value;
}

// Description:
// Get the address of a particular data index. Make sure data is allocated
// for the number of items requested. Set MaxId according to the number of
// data values requested.
inline int *vtkIntArray::WritePointer(const int id, const int number)
{
  int newSize=id+number;
  if ( newSize > this->Size ) this->Resize(newSize);
  if ( (--newSize) > this->MaxId ) this->MaxId = newSize;
  return this->Array + id;
}

// Description:
// Insert data at a specified position in the array.
inline void vtkIntArray::InsertValue(const int id, const int i)
{
  if ( id >= this->Size ) this->Resize(id+1);
  this->Array[id] = i;
  if ( id > this->MaxId ) this->MaxId = id;
}

// Description:
// Insert data at the end of the array. Return its location in the array.
inline int vtkIntArray::InsertNextValue(const int i)
{
  this->InsertValue (++this->MaxId,i); 
  return this->MaxId;
}

// Description:
// Resize object to just fit data requirement. Reclaims extra memory.
inline void vtkIntArray::Squeeze() {this->Resize (this->MaxId+1);}

#endif
