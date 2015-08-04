/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAoSDataArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAoSDataArrayTemplate - vtkGenericDataArray specialization that
// stores data array in the traditional VTK memory layout where a 3 component is
// stored in contiguous memory as \c A1A2A2B1B2B3C1C2C3 ... where A,B,C,... are
// tuples.
// .SECTION Description
// This replaces vtkDataArrayTemplate.

#ifndef vtkAoSDataArrayTemplate_h
#define vtkAoSDataArrayTemplate_h

#include "vtkGenericDataArray.h"
#include "vtkBuffer.h"

template <class ValueTypeT>
class vtkAoSDataArrayTemplate :
  public vtkTypeTemplate<
          vtkAoSDataArrayTemplate<ValueTypeT>,
          vtkGenericDataArray<vtkAoSDataArrayTemplate<ValueTypeT>, ValueTypeT>
         >
{
  typedef vtkGenericDataArray<vtkAoSDataArrayTemplate<ValueTypeT>, ValueTypeT >
          GenericDataArrayType;
public:
  typedef GenericDataArrayType Superclass;
  typedef vtkAoSDataArrayTemplate<ValueTypeT> SelfType;
  typedef typename Superclass::ValueType ValueType;
  typedef typename Superclass::ReferenceType ReferenceType;

  // Description:
  // Legacy support for array-of-structs value iteration.
  // TODO Deprecate?
  typedef ValueType* Iterator;
  Iterator Begin() { return Iterator(this->GetVoidPointer(0)); }
  Iterator End() { return Iterator(this->GetVoidPointer(this->MaxId + 1)); }

  static vtkAoSDataArrayTemplate* New();

  // **************************************************************************
  // Methods that are needed to be implemented by every vtkGenericDataArray
  // subclass.
  // **************************************************************************
  inline const ReferenceType GetValue(vtkIdType valueIdx) const
    {
    return this->Buffer.GetBuffer()[valueIdx];
    }
  inline void GetTupleValue(vtkIdType tupleIdx, ValueType* tuple) const
    {
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
    std::copy(this->Buffer.GetBuffer() + valueIdx,
              this->Buffer.GetBuffer() + valueIdx + this->NumberOfComponents,
              tuple);
    }
  inline const ReferenceType GetComponentValue(vtkIdType index, int comp) const
    {
    return this->Buffer.GetBuffer()[this->NumberOfComponents*index + comp];
    }
  inline void SetValue(vtkIdType valueIdx, ValueType value)
    {
    this->Buffer.GetBuffer()[valueIdx] = value;
    this->DataChanged();
    }
  inline void SetTupleValue(vtkIdType tupleIdx, const ValueType* tuple)
    {
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
    std::copy(tuple, tuple + this->NumberOfComponents,
              this->Buffer.GetBuffer() + valueIdx);
    this->DataChanged();
    }
  inline void SetComponentValue(vtkIdType tupleIdx, int comp, ValueType value)
    {
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents + comp;
    this->SetValue(valueIdx, value);
    }

  // **************************************************************************
  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. Set MaxId according to the number of
  // data values requested.
  ValueType* WritePointer(vtkIdType id, vtkIdType number);
  virtual void* WriteVoidPointer(vtkIdType id, vtkIdType number)
    { return this->WritePointer(id, number); }

  // Description:
  // Get the address of a particular data index. Performs no checks
  // to verify that the memory has been allocated etc.
  // If the data is simply being iterated over, consider using
  // vtkDataArrayIteratorMacro for safety and efficiency, rather than using this
  // member directly.
  ValueType* GetPointer(vtkIdType id) { return this->Buffer.GetBuffer() + id; }
  virtual void* GetVoidPointer(vtkIdType id) { return this->GetPointer(id); }

  enum DeleteMethod
    {
    VTK_DATA_ARRAY_FREE=vtkBuffer<ValueType>::VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE=vtkBuffer<ValueType>::VTK_DATA_ARRAY_DELETE
    };

  // Description:
  // This method lets the user specify data to be held by the array.  The
  // array argument is a pointer to the data.  size is the size of the
  // array supplied by the user.  Set save to 1 to keep the class from
  // deleting the array when it cleans up or reallocates memory.  The class
  // uses the actual array provided; it does not copy the data from the
  // suppled array. If specified, the delete method determines how the data
  // array will be deallocated. If the delete method is
  // VTK_DATA_ARRAY_FREE, free() will be used. If the delete method is
  // DELETE, delete[] will be used. The default is FREE.
  void SetArray(ValueType* array, vtkIdType size, int save, int deleteMethod);
  void SetArray(ValueType* array, vtkIdType size, int save)
    { this->SetArray(array, size, save, VTK_DATA_ARRAY_FREE); }
  virtual void SetVoidArray(void* array, vtkIdType size, int save)
    { this->SetArray(static_cast<ValueType*>(array), size, save); }
  virtual void SetVoidArray(void* array, vtkIdType size, int save,
                            int deleteMethod)
    {
    this->SetArray(static_cast<ValueType*>(array), size, save, deleteMethod);
    }

  // Description:
  // Tell the array explicitly that a single data element has
  // changed. Like DataChanged(), then is only necessary when you
  // modify the array contents without using the array's API.
  // @note This is a legacy method from vtkDataArrayTemplate, and is only
  // implemented for array-of-struct arrays. It currently just calls
  // DataChanged() and does nothing clever.
  virtual void DataElementChanged(vtkIdType)
  {
    this->DataChanged();
  }

//BTX
protected:
  vtkAoSDataArrayTemplate();
  ~vtkAoSDataArrayTemplate();

  // **************************************************************************
  // Methods that are needed to be implemented by every vtkGenericDataArray
  // subclass.
  // **************************************************************************
  // Implement the memory management interface.
  bool AllocateTuples(vtkIdType numTuples);
  bool ReallocateTuples(vtkIdType numTuples);
  // **************************************************************************

  vtkBuffer<ValueType> Buffer;
  ValueType ValueRange[2]; // XXX
  bool SaveUserArray;
  int DeleteMethod;

private:
  vtkAoSDataArrayTemplate(const vtkAoSDataArrayTemplate&); // Not implemented.
  void operator=(const vtkAoSDataArrayTemplate&); // Not implemented.
  friend class vtkGenericDataArray<vtkAoSDataArrayTemplate<ValueTypeT>,
                                   ValueTypeT>;
//ETX
};

#include "vtkAoSDataArrayTemplate.txx"
#endif
