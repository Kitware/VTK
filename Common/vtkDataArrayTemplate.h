/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataArrayTemplate - Implementation template for vtkDataArray.
// .SECTION Description
// There is a vtkDataArray subclass for each native type supported by
// VTK.  This template is used to implement all the subclasses in the
// same way while avoiding code duplication.

#ifndef __vtkDataArrayTemplate_h
#define __vtkDataArrayTemplate_h

#include "vtkDataArray.h"

template <class T>
class vtkDataArrayTemplate: public vtkDataArray
{
public:
  typedef vtkDataArray Superclass;
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  int Allocate(vtkIdType sz, vtkIdType ext=1000);

  // Description:
  // Release storage and reset array to initial state.
  void Initialize();

  // Description:
  // Return the size of the data type.
  int GetDataTypeSize() { return static_cast<int>(sizeof(T)); }

  // Description:
  // Set the number of n-tuples in the array.
  void SetNumberOfTuples(vtkIdType number);

  // Description:
  // Get a pointer to a tuple at the ith location. This is a dangerous method
  // (it is not thread safe since a pointer is returned).
  double* GetTuple(vtkIdType i);

  // Description:
  // Copy the tuple value into a user-provided array.
  void GetTuple(vtkIdType i, double* tuple);
  void GetTupleValue(vtkIdType i, T* tuple);

  // Description:
  // Set the tuple value at the ith location in the array.
  void SetTuple(vtkIdType i, const float* tuple);
  void SetTuple(vtkIdType i, const double* tuple);
  void SetTupleValue(vtkIdType i, const T* tuple);

  // Description:
  // Insert (memory allocation performed) the tuple into the ith location
  // in the array.
  void InsertTuple(vtkIdType i, const float* tuple);
  void InsertTuple(vtkIdType i, const double* tuple);
  void InsertTupleValue(vtkIdType i, const T* tuple);

  // Description:
  // Insert (memory allocation performed) the tuple onto the end of the array.
  vtkIdType InsertNextTuple(const float* tuple);
  vtkIdType InsertNextTuple(const double* tuple);
  vtkIdType InsertNextTupleValue(const T* tuple);

  // Description:
  // Resize object to just fit data requirement. Reclaims extra memory.
  void Squeeze() { this->ResizeAndExtend (this->MaxId+1); }

  // Description:
  // Resize the array while conserving the data.
  virtual void Resize(vtkIdType numTuples);

  // Description:
  // Get the data at a particular index.
  T GetValue(vtkIdType id) { return this->Array[id]; }

  // Description:
  // Set the data at a particular index. Does not do range checking. Make sure
  // you use the method SetNumberOfValues() before inserting data.
  void SetValue(vtkIdType id, T value)
    { this->Array[id] = value;};

  // Description:
  // Specify the number of values for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetValue() method for fast insertion.
  void SetNumberOfValues(vtkIdType number);

  // Description:
  // Insert data at a specified position in the array.
  void InsertValue(vtkIdType id, T f);

  // Description:
  // Insert data at the end of the array. Return its location in the array.
  vtkIdType InsertNextValue(T f);

  // Description:
  // Return the data component at the ith tuple and jth component location.
  // Note that i is less then NumberOfTuples and j is less then
  // NumberOfComponents.
  double GetComponent(vtkIdType i, int j);

  // Description:
  // Set the data component at the ith tuple and jth component location.
  // Note that i is less then NumberOfTuples and j is less then
  // NumberOfComponents. Make sure enough memory has been allocated
  // (use SetNumberOfTuples() and SetNumberOfComponents()).
  void SetComponent(vtkIdType i, int j, double c);

  // Description:
  // Insert the data component at ith tuple and jth component location.
  // Note that memory allocation is performed as necessary to hold the data.
  virtual void InsertComponent(vtkIdType i, int j, double c);

  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  T* WritePointer(vtkIdType id, vtkIdType number);
  void* WriteVoidPointer(vtkIdType id, vtkIdType number)
    { return this->WritePointer(id, number); }

  // Description:
  // Get the address of a particular data index. Performs no checks
  // to verify that the memory has been allocated etc.
  T* GetPointer(vtkIdType id) { return this->Array + id; }
  void* GetVoidPointer(vtkIdType id) { return this->GetPointer(id); }

  // Description:
  // Deep copy of another double array.
  void DeepCopy(vtkDataArray* da);

  // Description:
  // This method lets the user specify data to be held by the array.  The
  // array argument is a pointer to the data.  size is the size of
  // the array supplied by the user.  Set save to 1 to keep the class
  // from deleting the array when it cleans up or reallocates memory.
  // The class uses the actual array provided; it does not copy the data
  // from the suppled array.
  void SetArray(T* array, vtkIdType size, int save);
  void SetVoidArray(void* array, vtkIdType size, int save)
    { this->SetArray(static_cast<T*>(array), size, save); }

  // Description:
  // This method copies the array data to the void pointer specified
  // by the user.  It is up to the user to allocate enough memory for
  // the void pointer.
  void ExportToVoidPointer(void *out_ptr);

  // Description:
  // Do not call.  Use GetRange.
  virtual void ComputeRange(int comp);
protected:
  vtkDataArrayTemplate(vtkIdType numComp);
  ~vtkDataArrayTemplate();

  T* Array;   // pointer to data
  T* ResizeAndExtend(vtkIdType sz);  // function to resize data

  int TupleSize; //used for data conversion
  double* Tuple;

  int SaveUserArray;

  void ComputeScalarRange(int comp);
  void ComputeVectorRange();
private:
  vtkDataArrayTemplate(const vtkDataArrayTemplate&);  // Not implemented.
  void operator=(const vtkDataArrayTemplate&);  // Not implemented.
};

#if !defined(VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION)
# define VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(T) \
   template class VTK_COMMON_EXPORT vtkDataArrayTemplate< T >
#else
# include "vtkDataArrayTemplateImplicit.txx"
# define VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(T)
#endif

#endif // !defined(__vtkDataArrayTemplate_h)

// This portion must be OUTSIDE the include blockers.  Each
// vtkDataArray subclass uses this to give its instantiation of this
// template a DLL interface.
#if defined(VTK_DATA_ARRAY_TEMPLATE_TYPE)
# if defined(VTK_BUILD_SHARED_LIBS) && defined(_MSC_VER)
#  pragma warning (push)
#  pragma warning (disable: 4091) // warning C4091: 'extern ' : 
   // ignored on left of 'int' when no variable is declared
#  pragma warning (disable: 4231) // Compiler-specific extension warning.
   // Use an "extern explicit instantiation" to give the class a DLL
   // interface.  This is a compiler-specific extension.
   extern VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(VTK_DATA_ARRAY_TEMPLATE_TYPE);
#  pragma warning (pop)
# endif
# undef VTK_DATA_ARRAY_TEMPLATE_TYPE
#endif
