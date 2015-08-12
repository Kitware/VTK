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
  inline const ReferenceType GetValue(vtkIdType valueIdx) const
    {
    // TODO this method shadows a non-const method in vtkDataArrayTemplate
    // that returns a value. Prolly won't matter for most cases, but we should
    // note it somewhere.
    return static_cast<const DerivedT*>(this)->GetValue(valueIdx);
    }
  inline void GetTupleValue(vtkIdType tupleIdx, ValueType* tuple) const
    {
    static_cast<const DerivedT*>(this)->GetTupleValue(tupleIdx, tuple);
    }
  inline const ReferenceType GetComponentValue(vtkIdType tupleIdx,
                                               int comp) const
    {
    return static_cast<const DerivedT*>(this)->GetComponentValue(tupleIdx,
                                                                 comp);
    }
  inline void SetValue(vtkIdType valueIdx, ValueType value)
    {
    static_cast<DerivedT*>(this)->SetValue(valueIdx, value);
    }
  inline void SetTupleValue(vtkIdType tupleIdx, const ValueType* tuple)
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
  ValueType* GetPointer(vtkIdType id)
  {
    return static_cast<ValueType*>(this->GetVoidPointer(id));
  }

  // Description:
  // Implementation provided simply raises a runtime error. If subclasses are
  // keep on supporting this API, they should override the method.
  virtual void SetVoidArray(void*, vtkIdType, int);

  // Description:
  // Implementation provided simply raises a runtime error. If subclasses are
  // keep on supporting this API, they should override the method.
  virtual void* WriteVoidPointer(vtkIdType id, vtkIdType number);
  ValueType* WritePointer(vtkIdType id, vtkIdType number)
  {
    return static_cast<ValueType*>(this->WriteVoidPointer(id, number));
  }

  //----------------------------------------------------------------------------
  // Methods relating to memory allocated for this array.

  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  virtual int Allocate(vtkIdType size, vtkIdType ext = 1000);
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

    // Update MaxId if we truncated:
    if ((this->Size - 1) < this->MaxId)
      {
      this->MaxId = (this->Size - 1);
      }

    return 1;
    }

  virtual void SetNumberOfComponents(int num)
    {
    this->vtkDataArray::SetNumberOfComponents(num);
    this->LegacyTuple.resize(num);
    }

  virtual void SetNumberOfTuples(vtkIdType number)
    {
    vtkIdType newSize = number * this->NumberOfComponents;
    if (this->Allocate(newSize, 0))
      {
      this->MaxId = newSize - 1;
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
  // Reimplemented for efficiency -- base impl allocates heap memory:
  virtual void InsertComponent(vtkIdType tupleIdx, int compIdx, double val)
    {
    // Update MaxId to the inserted component (not the complete tuple) for
    // compatibility with InsertNextValue.
    vtkIdType newMaxId = std::max(tupleIdx * this->NumberOfComponents + compIdx,
                                  this->MaxId);
    this->EnsureAccessToTuple(tupleIdx);
    assert("Sufficient space allocated." && this->MaxId >= newMaxId);
    this->MaxId = newMaxId;
    this->SetComponent(tupleIdx, compIdx, val);
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
  // SetComponent methods.
  virtual void SetComponent(vtkIdType i, int j, double c)
  {
    // Reimplemented for efficiency (base impl allocates heap memory)
    this->SetComponentValue(i, j, static_cast<ValueType>(c));
  }

  //----------------------------------------------------------------------------
  // GetComponent methods.
  virtual double GetComponent(vtkIdType i, int j)
  {
    // Reimplemented for efficiency (base impl allocates heap memory)
    return static_cast<double>(this->GetComponentValue(i, j));
  }

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
  // Insert the variant's value at the specified value index.
  virtual void InsertVariantValue(vtkIdType idx, vtkVariant value);

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
  virtual vtkArrayIterator* NewIterator();

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

    // Extending array without needing to reallocate:
    if (this->MaxId < nextValueIdx)
      {
      this->MaxId = nextValueIdx;
      }

    this->SetValue(nextValueIdx, v);
    return nextValueIdx;
    }

  // Description:
  // Insert data at a specified position in the array.
  void InsertValue(vtkIdType idx, ValueType v)
    {
    vtkIdType tuple = idx / this->NumberOfComponents;
    // Update MaxId to the inserted component (not the complete tuple) for
    // compatibility with InsertNextValue.
    vtkIdType newMaxId = std::max(idx, this->MaxId);
    if (this->EnsureAccessToTuple(tuple))
      {
      assert("Sufficient space allocated." && this->MaxId >= newMaxId);
      this->MaxId = newMaxId;
      this->SetValue(idx, v);
      }
    }

  // Description:
  // Insert (memory allocation performed) the tuple into the ith location
  // in the array.
  void InsertTupleValue(vtkIdType tupleIdx, const ValueType *t)
    {
    if (this->EnsureAccessToTuple(tupleIdx))
      {
      this->SetTupleValue(tupleIdx, t);
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
  // Insert (memory allocation performed) the value at the specified tuple and
  // component location.
  void InsertComponentValue(vtkIdType tupleIdx, int compIdx, ValueType val)
    {
    // Update MaxId to the inserted component (not the complete tuple) for
    // compatibility with InsertNextValue.
    vtkIdType newMaxId = tupleIdx * this->NumberOfComponents + compIdx;
    if (this->MaxId > newMaxId)
      {
      newMaxId = this->MaxId;
      }
    this->EnsureAccessToTuple(tupleIdx);
    assert("Sufficient space allocated." && this->MaxId >= newMaxId);
    this->MaxId = newMaxId;
    this->SetComponentValue(tupleIdx, compIdx, val);
    }

  // Description:
  // Returns the number of values i.e.
  // (this->NumberOfComponents*this->NumberOfTuples)
  // TODO this can go in vtkAbstractArray.
  inline vtkIdType GetNumberOfValues()
    {
    return (this->MaxId + 1);
    }

  // Description:
  // Get the range of array values for the given component in the
  // native data type.
  void GetValueRange(ValueType range[2], int comp)
    {
    // TODO This is how vtkDataArrayTemplate implemented this. It should be
    // reimplemented to avoid truncation of e.g. longer integers.
    double doubleRange[2];
    this->ComputeRange(doubleRange, comp);
    range[0] = static_cast<ValueType>(doubleRange[0]);
    range[1] = static_cast<ValueType>(doubleRange[1]);
    }
  ValueType *GetValueRange(int comp)
    {
    this->LegacyValueRange.resize(2);
    this->GetValueRange(&this->LegacyValueRange[0], comp);
    return &this->LegacyValueRange[0];
    }

  // Description:
  // Get the range of array values for the 0th component in the
  // native data type.
  ValueType *GetValueRange() { return this->GetValueRange(0); }
  void GetValueRange(ValueType range[2]) { this->GetValueRange(range, 0); }

  // Description:
  // Return the capacity in typeof T units of the current array.
  // TODO Redundant with GetSize. Deprecate?
  vtkIdType Capacity() { return this->Size; }

protected:
  vtkGenericDataArray()
    : Lookup(*this)
    {
    // Initialize internal data structures:
    this->SetNumberOfComponents(this->NumberOfComponents);
    }

  virtual ~vtkGenericDataArray()
    {
    }

  // This method resizes the array if needed so that the given tuple index is
  // valid/accessible.
  bool EnsureAccessToTuple(vtkIdType tupleIdx)
    {
    if (tupleIdx < 0)
      {
      return false;
      }
    vtkIdType minSize = (1 + tupleIdx) * this->NumberOfComponents;
    vtkIdType expectedMaxId = minSize - 1;
    if (this->MaxId < expectedMaxId)
      {
      if (this->Size < minSize)
        {
        if (!this->Resize(tupleIdx + 1))
          {
          return false;
          }
        }
      this->MaxId = expectedMaxId;
      }
    return true;
    }

  vtkGenericDataArrayLookupHelper<SelfType> Lookup;
private:
  vtkGenericDataArray(const vtkGenericDataArray&); // Not implemented.
  void operator=(const vtkGenericDataArray&); // Not implemented.
  std::vector<double> LegacyTuple;
  std::vector<ValueType> LegacyValueRange;
};

#include "vtkGenericDataArray.txx"
#include "vtkGenericDataArrayMacros.h"
#endif
