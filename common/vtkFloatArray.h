/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatArray.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkFloatArray - dynamic, self-adjusting floating point array
// .SECTION Description
// vtkFloatArray is an array of floating point numbers. It provides methods
// for insertion and retrieval of floating point values, and will 
// automatically resize itself to hold new data.

#ifndef __vtkFloatArray_h
#define __vtkFloatArray_h

#include "vtkDataArray.h"

class VTK_EXPORT vtkFloatArray : public vtkDataArray
{
public:
  static vtkFloatArray *New();
  vtkTypeMacro(vtkFloatArray,vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  int Allocate(const int sz, const int ext=1000);

  // Description:
  // Release storage and reset array to initial state.
  void Initialize();

  // Description:
  // Create the same type object as this (virtual constructor).
  vtkDataArray *MakeObject();

  // Description:
  // Get the data type.
  int GetDataType() 
    {return VTK_FLOAT;}

  // Description:
  // Set the number of n-tuples in the array.
  void SetNumberOfTuples(const int number);

  // Description:
  // Get a pointer to a tuple at the ith location.
  float *GetTuple(const int i);

  // Description:
  // Copy the tuple value into a user-provided array.
  void GetTuple(const int i, float * tuple);
  void GetTuple(const int i, double * tuple);

  // Description:
  // Set the tuple value at the ith location in the array.
  void SetTuple(const int i, const float * tuple);
  void SetTuple(const int i, const double * tuple);

  // Description:
  // Insert (memory allocation performed) the tuple into the ith location
  // in the array.
  void InsertTuple(const int i, const float * tuple);
  void InsertTuple(const int i, const double * tuple);

  // Description:
  // Insert (memory allocation performed) the tuple onto the end of the array.
  int InsertNextTuple(const float * tuple);
  int InsertNextTuple(const double * tuple);

  // Description:
  // Resize object to just fit data requirement. Reclaims extra memory.
  void Squeeze() {this->ResizeAndExtend(this->MaxId+1);};

  // Description:
  // Resize the array while conserving the data.
  virtual void Resize(int numTuples);

  // Description:
  // Return the data component at the ith tuple and jth component location.
  // Note that i is less than NumberOfTuples and j is less than 
  // NumberOfComponents.
  float GetComponent(const int i, const int j);

  // Description:
  // Return the active data component at the ith tuple location
  // by calling GetComponent(i, this->ActiveComponent).
  virtual float GetComponent(const int i) 
    { return this->vtkDataArray::GetComponent(i); }

  // Description:
  // Set the data component at the ith tuple and jth component location.
  // Note that i is less than NumberOfTuples and j is less than
  // NumberOfComponents. Make sure enough memory has been allocated 
  // (use SetNumberOfTuples() and SetNumberOfComponents()).
  void SetComponent(const int i, const int j, const float c);

  // Description:
  // Set the active data component at the ith tuple location
  // by calling SetComponent(i, this->ActiveComponent, c)
  virtual void SetComponent(const int i, const float c)
    { this->vtkDataArray::SetComponent(i, c); }

  // Description:
  // Insert the data component at ith tuple and jth component location. 
  // Note that memory allocation is performed as necessary to hold the data.
  void InsertComponent(const int i, const int j, const float c);

  // Description:
  // Get the data at a particular index.
  float GetValue(const int id) 
    {return this->Array[id];}

  // Description:
  // Set the data at a particular index. Does not do range checking. Make sure
  // you use the method SetNumberOfValues() before inserting data.
  void SetValue(const int id, const float value) 
    {this->Array[id] = value;}

  // Description:
  // Specify the number of values for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetValue() method for fast insertion.
  void SetNumberOfValues(const int number);

  // Description:
  // Insert data at a specified position in the array.
  void InsertValue(const int id, const float f);

  // Description:
  // Insert data at the end of the array. Return its location in the array.
  int InsertNextValue(const float f);

  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  float *WritePointer(const int id, const int number);

  // Description:
  // Get the address of a particular data index. Performs no checks
  // to verify that the memory has been allocated etc.
  void *GetVoidPointer(const int id) 
    {return (void *)this->GetPointer(id);}
  float *GetPointer(const int id) 
    {return this->Array + id;}

  // Description:
  // Deep copy of another float array.
  void DeepCopy(vtkDataArray *fa);

  // Description:
  // This method lets the user specify data to be held by the array.  The 
  // array argument is a pointer to the data.  size is the size of 
  // the array supplied by the user.  Set save to 1 to keep the class
  // from deleting the array when it cleans up or reallocates memory.
  // The class uses the actual array provided; it does not copy the data 
  // from the suppled array.
  void SetArray(float* array, int size, int save);
  void SetVoidArray(void *array,int size, int save) 
    {this->SetArray((float*)array, size, save);}

#ifndef VTK_REMOVE_LEGACY_CODE
  // Description:
  // For legacy compatibility. Do not use.
  void DeepCopy(vtkDataArray &fa) 
    {VTK_LEGACY_METHOD(DeepCopy,"3.2"); this->DeepCopy(&fa);}
#endif
  
  
protected:
  vtkFloatArray(int numComp=1);
  ~vtkFloatArray();
  vtkFloatArray(const vtkFloatArray&) {};
  void operator=(const vtkFloatArray&) {};

  float *Array;  // pointer to data
  float *ResizeAndExtend(const int sz);  // function to reallocate data

  int SaveUserArray;
};

inline void vtkFloatArray::SetNumberOfValues(const int number) 
{
  this->Allocate(number);
  this->MaxId = number - 1;
}


inline float *vtkFloatArray::WritePointer(const int id, const int number) 
{
  int newSize=id+number;
  if ( newSize > this->Size )
    {
    this->ResizeAndExtend(newSize);
    }
  if ( (--newSize) > this->MaxId )
    {
    this->MaxId = newSize;
    }
  return this->Array + id;
}

inline void vtkFloatArray::InsertValue(const int id, const float f)
{
  if ( id >= this->Size )
    {
    this->ResizeAndExtend(id+1);
    }
  this->Array[id] = f;
  if ( id > this->MaxId )
    {
    this->MaxId = id;
    }
}

inline int vtkFloatArray::InsertNextValue(const float f)
{
  this->InsertValue (++this->MaxId,f); 
  return this->MaxId;
}

#endif
