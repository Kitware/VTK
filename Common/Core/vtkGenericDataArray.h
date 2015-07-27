/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericDataArray
// .SECTION Description

#ifndef vtkGenericDataArray_h
#define vtkGenericDataArray_h

#include "vtkDataArray.h"
#include "vtkSmartPointer.h"
#include "vtkTypeTemplate.h"
#include "vtkTypeTraits.h"
#include "vtkGenericDataArrayLookupHelper.h"
#include "vtkGenericDataArrayHelper.h"

#include <cassert>

template<class DerivedT,
         class ValueTypeT,
         class ReferenceTypeT=ValueTypeT&>
class vtkGenericDataArray : public vtkTypeTemplate<
                     vtkGenericDataArray<DerivedT, ValueTypeT, ReferenceTypeT>,
                     vtkDataArray>
{
  typedef
    vtkGenericDataArray<DerivedT, ValueTypeT, ReferenceTypeT> SelfType;
public:
  typedef ValueTypeT     ValueType;
  typedef ReferenceTypeT ReferenceType;

  //----------------------------------------------------------------------------
  // Methods that must be defined by the subclasses.
  // Let's call these GenericDataArray concept methods.
  inline ReferenceType GetValue(vtkIdType valueIdx) const
    {
    return static_cast<const DerivedT*>(this)->GetValue(valueIdx);
    }
  inline void GetTupleValue(vtkIdType tupleIdx, ValueType* tuple) const
    {
    static_cast<const DerivedT*>(this)->GetTupleValue(tupleIdx, tuple);
    }
  inline ReferenceType GetComponentValue(vtkIdType tupleIdx, int comp) const
    {
    return static_cast<const DerivedT*>(this)->GetComponentValue(tupleIdx,
                                                                 comp);
    }
  inline void SetValue(vtkIdType valueIdx, ValueType value)
    {
    static_cast<DerivedT*>(this)->SetValue(valueIdx, value);
    }
  inline void SetTupleValue(vtkIdType tupleIdx, ValueType* tuple)
    {
    static_cast<DerivedT*>(this)->SetTupleValue(tupleIdx, tuple);
    }
  inline void SetComponentValue(vtkIdType tupleIdx, int comp, ValueType value)
    {
    static_cast<DerivedT*>(this)->SetComponentValue(tupleIdx, comp, value);
    }

  // Provide implementations for pure virtual methods in vtkDataArray.

  //----------------------------------------------------------------------------
  // Core methods.
  virtual int GetDataType()
    {
    return vtkTypeTraits<ValueType>::VTK_TYPE_ID;
    }
  virtual int GetDataTypeSize()
    {
    return static_cast<int>(sizeof(ValueType));
    }

  //----------------------------------------------------------------------------
  // Pointer access methods.
  // These are considered legacy and are not implemented. New arrays types keep
  // on supporting filters that use this API should override these methods to
  // provide appropriate implementations.

  // Description:
  // Default implementation raises a runtime error. If subclasses are keep on
  // supporting this API, they should override this method.
  virtual void *GetVoidPointer(vtkIdType id);

  // Description:
  // Implementation provided simply raises a runtime error. If subclasses are
  // keep on supporting this API, they should override the method.
  virtual void SetVoidArray(void*, vtkIdType, int);

  // Description:
  // Implementation provided simply raises a runtime error. If subclasses are
  // keep on supporting this API, they should override the method.
  virtual void* WriteVoidPointer(vtkIdType id, vtkIdType number);

  //----------------------------------------------------------------------------
  // Methods relating to memory allocated for this array.

  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  virtual int Allocate(vtkIdType size, vtkIdType vtkNotUsed(ext))
    {
    DerivedT* self = static_cast<DerivedT*>(this);

    // Allocator must updated this->Size and this->MaxId properly.
    this->MaxId = -1;
    if (size > this->Size)
      {
      this->Size = 0;

      // let's keep the size an integral multiple of the number of components.
      size = size < 0? 0 : size;
      int numComps = this->GetNumberOfComponents() > 0
          ? this->GetNumberOfComponents() : 1;
      vtkIdType numTuples = ceil(size/ static_cast<double>(numComps));
      // NOTE: if numTuples is 0, AllocateTuples is expected to release the
      // memory.
      if (self->AllocateTuples(numTuples) == false)
        {
        vtkErrorMacro("Unable to allocate " << size
                      << " elements of size " << sizeof(ValueType)
                      << " bytes. ");
#if !defined NDEBUG
        // We're debugging, crash here preserving the stack
        abort();
#elif !defined VTK_DONT_THROW_BAD_ALLOC
        // We can throw something that has universal meaning
        throw std::bad_alloc();
#else
        // We indicate that alloc failed by return
        return 0;
#endif
        }
      this->Size = numTuples * numComps;
      }
    this->DataChanged();
    return 1;
    }
  virtual int Resize(vtkIdType numTuples)
    {
    int numComps = this->GetNumberOfComponents();
    vtkIdType curNumTuples = this->Size / (numComps> 0? numComps : 1);
    if (numTuples > curNumTuples)
      {
      // Requested size is bigger than current size.  Allocate enough
      // memory to fit the requested size and be more than double the
      // currently allocated memory.
      numTuples = curNumTuples + numTuples;
      }
    else if (numTuples == curNumTuples)
      {
      return 1;
      }
    else
      {
      // Requested size is smaller than current size.  Squeeze the
      // memory.
      this->DataChanged();
      }

    assert(numTuples >= 0);

    DerivedT* self = static_cast<DerivedT*>(this);
    if (!self->ReallocateTuples(numTuples))
      {
      vtkErrorMacro("Unable to allocate " << numTuples * numComps
                    << " elements of size " << sizeof(ValueType)
                    << " bytes. ");
      #if !defined NDEBUG
      // We're debugging, crash here preserving the stack
      abort();
      #elif !defined VTK_DONT_THROW_BAD_ALLOC
      // We can throw something that has universal meaning
      throw std::bad_alloc();
      #else
      // We indicate that malloc failed by return
      return 0;
      #endif
      }

    // Allocation was successful. Save it.
    this->Size = numTuples * numComps;
    this->MaxId = (this->Size - 1);
    return 1;
    }

  virtual void SetNumberOfComponents(int num)
    {
    this->vtkDataArray::SetNumberOfComponents(num);
    this->LegacyTuple.resize(num);
    }

  virtual void SetNumberOfTuples(vtkIdType number)
    {
    if (this->Allocate(number*this->NumberOfComponents, 0))
      {
      this->MaxId = this->Size - 1;
      }
    }

  virtual void Initialize()
    {
    this->Resize(0);
    this->DataChanged();
    }
  virtual void Squeeze()
    {
    this->Resize(this->GetNumberOfTuples());
    }

  //----------------------------------------------------------------------------
  // Insert* methods. The call the Set* equivalent methods after having resized,
  // if needed.
  virtual void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source)
    {
    this->EnsureAccessToTuple(i);
    this->SetTuple(i, j, source);
    }
  virtual void InsertTuple(vtkIdType i, const float *source)
    {
    this->EnsureAccessToTuple(i);
    this->SetTuple(i, source);
    }
  virtual void InsertTuple(vtkIdType i, const double *source)
    {
    this->EnsureAccessToTuple(i);
    this->SetTuple(i, source);
    }
  virtual vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray *source)
    {
    vtkIdType nextTuple = this->GetNumberOfTuples();
    this->InsertTuple(nextTuple, j, source);
    return nextTuple;
    }
  virtual vtkIdType InsertNextTuple(const float *source)
    {
    vtkIdType nextTuple = this->GetNumberOfTuples();
    this->InsertTuple(nextTuple, source);
    return nextTuple;
    }
  virtual vtkIdType InsertNextTuple(const double *source)
    {
    vtkIdType nextTuple = this->GetNumberOfTuples();
    this->InsertTuple(nextTuple, source);
    return nextTuple;
    }
  virtual void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                            vtkAbstractArray *source);
  virtual void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                            vtkAbstractArray* source);

  //----------------------------------------------------------------------------
  // SetTuple methods.
  virtual void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source);
  virtual void SetTuple(vtkIdType i, const float *source);
  virtual void SetTuple(vtkIdType i, const double *source);

  //----------------------------------------------------------------------------
  // GetTuple methods.
  virtual double *GetTuple(vtkIdType i);
  virtual void GetTuple(vtkIdType i, double * tuple);

  //----------------------------------------------------------------------------
  // Description:
  // Removes a tuple at the given index. Default implementation
  // iterates over tuples to move elements. Subclasses are
  // encouraged to reimplemented this method to support faster implementations,
  // if needed.
  virtual void RemoveTuple(vtkIdType id);

  //----------------------------------------------------------------------------
  // Set Value methods. Note the index for all these methods is a "value" index
  // or component index assuming traditional VTK style memory layout for tuples
  // and components.
  virtual void SetVariantValue(vtkIdType idx, vtkVariant value);

  //----------------------------------------------------------------------------
  // All the lookup related methods We provide a default implementation that
  // works using the iterator. Since these methods are virtual, a subclass can
  // override these to provide faster alternatives.
  virtual vtkIdType LookupValue(vtkVariant value);
  virtual vtkIdType LookupTypedValue(ValueType value);
  virtual void LookupValue(vtkVariant value, vtkIdList* ids);
  virtual void LookupTypedValue(ValueType value, vtkIdList* ids);
  virtual void ClearLookup()
    {
    this->Lookup.ClearLookup();
    }

  virtual void DataChanged()
    {
    this->Lookup.ClearLookup();
    }

  //----------------------------------------------------------------------------
  // vtkArrayIterator API. This provides the generic vtkArrayIterator.
  // TODO:
  virtual vtkArrayIterator* NewIterator() { return NULL;}

  //----------------------------------------------------------------------------
  // API provided by vtkDataArrayTemplate/vtkTypedDataArray. These methods used
  // to be virtual. They are no longer virtual and the vtkGenericDataArray
  // concept methods defined above.

  // Description:
  // Get a reference to the scalar value at a particular index.
  // Index is value/component index assuming
  // traditional VTK memory layout (array-of-structures).
  ReferenceType GetValueReference(vtkIdType idx)
    {
    return this->GetValue(idx);
    }

  // Description:
  // Insert data at the end of the array. Return its location in the array.
  vtkIdType InsertNextValue(ValueType v)
    {
    vtkIdType nextValueIdx = this->MaxId + 1;
    if (nextValueIdx >= this->Size)
      {
      vtkIdType tuple = nextValueIdx / this->NumberOfComponents;
      this->EnsureAccessToTuple(tuple);
      // Since EnsureAccessToTuple will update the MaxId to point to the last
      // component in the last tuple, we move it back to support this method on
      // multi-component arrays.
      this->MaxId = nextValueIdx;
      }
    this->SetValue(nextValueIdx, v);
    }

  // Description:
  // Insert data at a specified position in the array.
  void InsertValue(vtkIdType idx, ValueType v)
    {
    vtkIdType tuple = idx / this->NumberOfComponents;
    if (this->EnsureAccessToTuple(tuple))
      {
      this->SetValue(idx, v);
      }
    }

  // Description:
  // Insert (memory allocation performed) the tuple into the ith location
  // in the array.
  void InsertTupleValue(vtkIdType idx, const ValueType *t)
    {
    vtkIdType tuple = idx / this->NumberOfComponents;
    if (this->EnsureAccessToTuple(tuple))
      {
      this->SetTupleValue(idx, t);
      }
    }

  // Description:
  // Insert (memory allocation performed) the tuple onto the end of the array.
  vtkIdType InsertNextTupleValue(const ValueType *t)
    {
    vtkIdType nextTuple = this->GetNumberOfTuples();
    this->InsertTupleValue(nextTuple, t);
    return nextTuple;
    }

  // Description:
  // Returns the number of values i.e.
  // (this->NumberOfComponents*this->NumberOfTuples)
  inline vtkIdType GetNumberOfValues()
    {
    return (this->MaxId + 1);
    }
protected:
  vtkGenericDataArray()
    : Lookup(*this)
    {
    }

  virtual ~vtkGenericDataArray()
    {
    }

  // This method resizes the array if needed so that the given tuple index is
  // valid/accessible.
  bool EnsureAccessToTuple(vtkIdType tupleIdx)
    {
    if (tupleIdx < 0) { return false; }
    vtkIdType expectedMaxId = (1 + tupleIdx) * this->NumberOfComponents - 1;
    if (this->MaxId < expectedMaxId)
      {
      return this->Resize(tupleIdx + 1) != 0;
      }
    return true;
    }

  vtkGenericDataArrayLookupHelper<SelfType> Lookup;
private:
  vtkGenericDataArray(const vtkGenericDataArray&); // Not implemented.
  void operator=(const vtkGenericDataArray&); // Not implemented.
  std::vector<double> LegacyTuple;
};

#include "vtkGenericDataArray.txx"
#include "vtkGenericDataArrayMacros.h"
#endif
