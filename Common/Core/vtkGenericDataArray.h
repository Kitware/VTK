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
// .NAME vtkGenericDataArray - Base interface for all typed vtkDataArray
// subclasses.
//
// .SECTION Description
//
// The vtkGenericDataArray class provides a generic implementation of the
// vtkDataArray API. It relies on subclasses providing access to data
// via 8 "concept methods", which should be implemented as non-virtual
// methods of the subclass. These methods are:
//
// - ValueType GetValue(vtkIdType valueIdx) const
// - [public] void SetValue(vtkIdType valueIdx, ValueType value)
// - [public] void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
// - [public] void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
// - [public] ValueType GetTypedComponent(vtkIdType tupleIdx, int compIdx) const
// - [public] void SetTypedComponent(vtkIdType tupleIdx, int compIdx,
//                                   ValueType value)
// - [protected] bool AllocateTuples(vtkIdType numTuples)
// - [protected] bool ReallocateTuples(vtkIdType numTuples)
//
// Note that these methods use the CRTP idiom, which provides static binding to
// avoid virtual calls. This allows the compiler to optimize away layers of
// indirection when these methods are used. Well-designed implementations
// of these methods will reduce to raw memory accesses, providing efficient
// performance comparable to working with the pointer data.
//
// See vtkAOSDataArrayTemplate and vtkSOADataArrayTemplate for example
// implementations.
//
// In practice, code should not be written to use vtkGenericDataArray objects.
// Doing so is rather unweildy due to the CRTP pattern requiring the derived
// class be provided as a template argument. Instead, the vtkArrayDispatch
// framework can be used to detect a vtkDataArray's implementation type and
// instantiate appropriate templated worker code.
//
// vtkArrayDispatch is also intended to replace code that currently relies on
// the encapsulation-breaking GetVoidPointer method. Not all subclasses of
// vtkDataArray use the memory layout assumed by GetVoidPointer; calling this
// method on, e.g. a vtkSOADataArrayTemplate will trigger a deep copy of the
// array data into an AOS buffer. This is very inefficient and should be
// avoided.
//
// .SECTION See Also
// vtkArrayDispatcher vtkDataArrayAccessor

#ifndef vtkGenericDataArray_h
#define vtkGenericDataArray_h

#include "vtkDataArray.h"
#include "vtkSmartPointer.h"
#include "vtkTypeTraits.h"
#include "vtkGenericDataArrayLookupHelper.h"

#include <cassert>

template<class DerivedT, class ValueTypeT>
class vtkGenericDataArray : public vtkDataArray
{
  typedef vtkGenericDataArray<DerivedT, ValueTypeT> SelfType;
public:
  typedef ValueTypeT ValueType;
  vtkTemplateTypeMacro(SelfType, vtkDataArray)

  /// @defgroup vtkGDAConceptMethods vtkGenericDataArray Concept Methods
  /// These signatures must be reimplemented in subclasses as public,
  /// non-virtual methods. Ideally, they should be inlined and as efficient as
  /// possible to ensure the best performance possible.

  // Description:
  // Get the value at @a valueIdx. @a valueIdx assumes AOS ordering.
  // @note GetTypedComponent is preferred over this method. It is faster for
  // SOA arrays, and shows equivalent performance for AOS arrays when
  // NumberOfComponents is known to the compiler (See vtkAssume.h).
  // @ingroup vtkGDAConceptMethods
  inline ValueType GetValue(vtkIdType valueIdx) const
  {
    return static_cast<const DerivedT*>(this)->GetValue(valueIdx);
  }

  // Description:
  // Set the value at @a valueIdx to @a value. @a valueIdx assumes AOS ordering.
  // @note SetTypedComponent is preferred over this method. It is faster for
  // SOA arrays, and shows equivalent performance for AOS arrays when
  // NumberOfComponents is known to the compiler (See vtkAssume.h).
  // @ingroup vtkGDAConceptMethods
  inline void SetValue(vtkIdType valueIdx, ValueType value)
  {
    static_cast<DerivedT*>(this)->SetValue(valueIdx, value);
  }

  // Description:
  // Copy the tuple at @a tupleIdx into @a tuple.
  // @note GetTypedComponent is preferred over this method. The overhead of
  // copying the tuple is significant compared to the more performant
  // component-wise access methods, which typically optimize to raw memory
  // access.
  // @ingroup vtkGDAConceptMethods
  inline void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
  {
    static_cast<const DerivedT*>(this)->GetTypedTuple(tupleIdx, tuple);
  }

  // Description:
  // Set this array's tuple at @a tupleIdx to the values in @a tuple.
  // @note SetTypedComponent is preferred over this method. The overhead of
  // copying the tuple is significant compared to the more performant
  // component-wise access methods, which typically optimize to raw memory
  // access.
  // @ingroup vtkGDAConceptMethods
  inline void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
  {
    static_cast<DerivedT*>(this)->SetTypedTuple(tupleIdx, tuple);
  }

  // Description:
  // Get component @a compIdx of the tuple at @a tupleIdx. This is typically
  // the fastest way to access array data.
  // @ingroup vtkGDAConceptMethods
  inline ValueType GetTypedComponent(vtkIdType tupleIdx, int compIdx) const
  {
    return static_cast<const DerivedT*>(this)->GetTypedComponent(tupleIdx,
                                                                 compIdx);
  }

  // Description:
  // Set component @a compIdx of the tuple at @a tupleIdx to @a value. This is
  // typically the fastest way to set array data.
  // @ingroup vtkGDAConceptMethods
  inline void SetTypedComponent(vtkIdType tupleIdx, int compIdx,
                                ValueType value)
  {
    static_cast<DerivedT*>(this)->SetTypedComponent(tupleIdx, compIdx, value);
  }

  // Description:
  // Default implementation raises a runtime error. If subclasses keep on
  // supporting this API, they should override this method.
  virtual void *GetVoidPointer(vtkIdType valueIdx);
  ValueType* GetPointer(vtkIdType valueIdx);
  virtual void SetVoidArray(void*, vtkIdType, int);
  virtual void SetVoidArray(void*, vtkIdType, int, int);
  virtual void* WriteVoidPointer(vtkIdType valueIdx, vtkIdType numValues);
  ValueType* WritePointer(vtkIdType valueIdx, vtkIdType numValues);

  // Description:
  // Removes a tuple at the given index. Default implementation
  // iterates over tuples to move elements. Subclasses are
  // encouraged to reimplemented this method to support faster implementations,
  // if needed.
  virtual void RemoveTuple(vtkIdType tupleIdx);

  // Description:
  // Insert data at the end of the array. Return its location in the array.
  vtkIdType InsertNextValue(ValueType value);

  // Description:
  // Insert data at a specified position in the array.
  void InsertValue(vtkIdType valueIdx, ValueType value);

  // Description:
  // Insert (memory allocation performed) the tuple t at tupleIdx.
  void InsertTypedTuple(vtkIdType tupleIdx, const ValueType *t);

  // Description:
  // Insert (memory allocation performed) the tuple onto the end of the array.
  vtkIdType InsertNextTypedTuple(const ValueType *t);

  // Description:
  // Insert (memory allocation performed) the value at the specified tuple and
  // component location.
  void InsertTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType val);

  // Description:
  // Get the range of array values for the given component in the
  // native data type.
  void GetValueRange(ValueType range[2], int comp);
  ValueType *GetValueRange(int comp);

  // Description:
  // Get the range of array values for the 0th component in the
  // native data type.
  ValueType *GetValueRange() { return this->GetValueRange(0); }
  void GetValueRange(ValueType range[2]) { this->GetValueRange(range, 0); }

  // Description:
  // Return the capacity in typeof T units of the current array.
  // TODO Leftover from vtkDataArrayTemplate, redundant with GetSize. Deprecate?
  vtkIdType Capacity() { return this->Size; }

  virtual int GetDataType();
  virtual int GetDataTypeSize();
  virtual bool HasStandardMemoryLayout();
  virtual int Allocate(vtkIdType size, vtkIdType ext = 1000);
  virtual int Resize(vtkIdType numTuples);
  virtual void SetNumberOfComponents(int num);
  virtual void SetNumberOfTuples(vtkIdType number);
  virtual void Initialize();
  virtual void Squeeze();
  virtual void InsertTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                           vtkAbstractArray *source);
  virtual void InsertTuple(vtkIdType tupleIdx, const float *source);
  virtual void InsertTuple(vtkIdType tupleIdx, const double *source);
  virtual void InsertComponent(vtkIdType tupleIdx, int compIdx, double value);
  virtual vtkIdType InsertNextTuple(vtkIdType srcTupleIdx,
                                    vtkAbstractArray *source);
  virtual vtkIdType InsertNextTuple(const float *tuple);
  virtual vtkIdType InsertNextTuple(const double *tuple);
  virtual double *GetTuple(vtkIdType tupleIdx);
  virtual void GetTuple(vtkIdType tupleIdx, double * tuple);
  virtual void SetComponent(vtkIdType tupleIdx, int compIdx, double value);
  virtual double GetComponent(vtkIdType tupleIdx, int compIdx);
  virtual void SetVariantValue(vtkIdType valueIdx, vtkVariant value);
  virtual vtkVariant GetVariantValue(vtkIdType valueIdx);
  virtual void InsertVariantValue(vtkIdType valueIdx, vtkVariant value);
  virtual vtkIdType LookupValue(vtkVariant value);
  virtual vtkIdType LookupTypedValue(ValueType value);
  virtual void LookupValue(vtkVariant value, vtkIdList* valueIds);
  virtual void LookupTypedValue(ValueType value, vtkIdList* valueIds);
  virtual void ClearLookup();
  virtual void DataChanged();
  virtual vtkArrayIterator* NewIterator();

protected:
  vtkGenericDataArray();
  virtual ~vtkGenericDataArray();

  // Description:
  // Allocate space for numTuples. Old data is not preserved. If numTuples == 0,
  // all data is freed.
  // @ingroup vtkGDAConceptMethods
  inline bool AllocateTuples(vtkIdType numTuples)
  {
    return static_cast<DerivedT*>(this)->AllocateTuples(numTuples);
  }

  // Description:
  // Allocate space for numTuples. Old data is preserved. If numTuples == 0,
  // all data is freed.
  // @ingroup vtkGDAConceptMethods
  inline bool ReallocateTuples(vtkIdType numTuples)
  {
    return static_cast<DerivedT*>(this)->ReallocateTuples(numTuples);
  }

  // This method resizes the array if needed so that the given tuple index is
  // valid/accessible.
  bool EnsureAccessToTuple(vtkIdType tupleIdx);

  vtkGenericDataArrayLookupHelper<SelfType> Lookup;
private:
  vtkGenericDataArray(const vtkGenericDataArray&); // Not implemented.
  void operator=(const vtkGenericDataArray&); // Not implemented.

  std::vector<double> LegacyTuple;
  std::vector<ValueType> LegacyValueRange;
};

#include "vtkGenericDataArray.txx"
#endif
// VTK-HeaderTest-Exclude: vtkGenericDataArray.h
