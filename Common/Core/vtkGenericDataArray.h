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
/**
 * @class   vtkGenericDataArray
 * @brief   Base interface for all typed vtkDataArray
 * subclasses.
 *
 *
 *
 * A more detailed description of this class and related tools can be found
 * \ref VTK-7-1-ArrayDispatch "here".
 *
 * The vtkGenericDataArray class provides a generic implementation of the
 * vtkDataArray API. It relies on subclasses providing access to data
 * via 8 "concept methods", which should be implemented as non-virtual
 * methods of the subclass. These methods are:
 *
 * - ValueType GetValue(vtkIdType valueIdx) const
 * - [public] void SetValue(vtkIdType valueIdx, ValueType value)
 * - [public] void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
 * - [public] void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
 * - [public] ValueType GetTypedComponent(vtkIdType tupleIdx, int compIdx) const
 * - [public] void SetTypedComponent(vtkIdType tupleIdx, int compIdx,
 *                                   ValueType value)
 * - [protected] bool AllocateTuples(vtkIdType numTuples)
 * - [protected] bool ReallocateTuples(vtkIdType numTuples)
 *
 * Note that these methods use the CRTP idiom, which provides static binding to
 * avoid virtual calls. This allows the compiler to optimize away layers of
 * indirection when these methods are used. Well-designed implementations
 * of these methods will reduce to raw memory accesses, providing efficient
 * performance comparable to working with the pointer data.
 *
 * See vtkAOSDataArrayTemplate and vtkSOADataArrayTemplate for example
 * implementations.
 *
 * In practice, code should not be written to use vtkGenericDataArray objects.
 * Doing so is rather unweildy due to the CRTP pattern requiring the derived
 * class be provided as a template argument. Instead, the vtkArrayDispatch
 * framework can be used to detect a vtkDataArray's implementation type and
 * instantiate appropriate templated worker code.
 *
 * vtkArrayDispatch is also intended to replace code that currently relies on
 * the encapsulation-breaking GetVoidPointer method. Not all subclasses of
 * vtkDataArray use the memory layout assumed by GetVoidPointer; calling this
 * method on, e.g. a vtkSOADataArrayTemplate will trigger a deep copy of the
 * array data into an AOS buffer. This is very inefficient and should be
 * avoided.
 *
 * @sa
 * vtkArrayDispatcher vtkDataArrayAccessor
*/

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

  /**
   * Compile time access to the VTK type identifier.
   */
  enum { VTK_DATA_TYPE = vtkTypeTraits<ValueType>::VTK_TYPE_ID };

  /// @defgroup vtkGDAConceptMethods vtkGenericDataArray Concept Methods
  /// These signatures must be reimplemented in subclasses as public,
  /// non-virtual methods. Ideally, they should be inlined and as efficient as
  /// possible to ensure the best performance possible.

  /**
   * Get the value at @a valueIdx. @a valueIdx assumes AOS ordering.
   * @note GetTypedComponent is preferred over this method. It is faster for
   * SOA arrays, and shows equivalent performance for AOS arrays when
   * NumberOfComponents is known to the compiler (See vtkAssume.h).
   * @ingroup vtkGDAConceptMethods
   */
  inline ValueType GetValue(vtkIdType valueIdx) const
  {
    return static_cast<const DerivedT*>(this)->GetValue(valueIdx);
  }

  /**
   * Set the value at @a valueIdx to @a value. @a valueIdx assumes AOS ordering.
   * @note SetTypedComponent is preferred over this method. It is faster for
   * SOA arrays, and shows equivalent performance for AOS arrays when
   * NumberOfComponents is known to the compiler (See vtkAssume.h).
   * @ingroup vtkGDAConceptMethods
   */
  inline void SetValue(vtkIdType valueIdx, ValueType value)
  {
    static_cast<DerivedT*>(this)->SetValue(valueIdx, value);
  }

  /**
   * Copy the tuple at @a tupleIdx into @a tuple.
   * @note GetTypedComponent is preferred over this method. The overhead of
   * copying the tuple is significant compared to the more performant
   * component-wise access methods, which typically optimize to raw memory
   * access.
   * @ingroup vtkGDAConceptMethods
   */
  inline void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
  {
    static_cast<const DerivedT*>(this)->GetTypedTuple(tupleIdx, tuple);
  }

  /**
   * Set this array's tuple at @a tupleIdx to the values in @a tuple.
   * @note SetTypedComponent is preferred over this method. The overhead of
   * copying the tuple is significant compared to the more performant
   * component-wise access methods, which typically optimize to raw memory
   * access.
   * @ingroup vtkGDAConceptMethods
   */
  inline void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
  {
    static_cast<DerivedT*>(this)->SetTypedTuple(tupleIdx, tuple);
  }

  /**
   * Get component @a compIdx of the tuple at @a tupleIdx. This is typically
   * the fastest way to access array data.
   * @ingroup vtkGDAConceptMethods
   */
  inline ValueType GetTypedComponent(vtkIdType tupleIdx, int compIdx) const
  {
    return static_cast<const DerivedT*>(this)->GetTypedComponent(tupleIdx,
                                                                 compIdx);
  }

  /**
   * Set component @a compIdx of the tuple at @a tupleIdx to @a value. This is
   * typically the fastest way to set array data.
   * @ingroup vtkGDAConceptMethods
   */
  inline void SetTypedComponent(vtkIdType tupleIdx, int compIdx,
                                ValueType value)
  {
    static_cast<DerivedT*>(this)->SetTypedComponent(tupleIdx, compIdx, value);
  }

  //@{
  /**
   * Default implementation raises a runtime error. If subclasses keep on
   * supporting this API, they should override this method.
   */
  void *GetVoidPointer(vtkIdType valueIdx) VTK_OVERRIDE;
  ValueType* GetPointer(vtkIdType valueIdx);
  void SetVoidArray(void*, vtkIdType, int) VTK_OVERRIDE;
  void SetVoidArray(void*, vtkIdType, int, int) VTK_OVERRIDE;
  void* WriteVoidPointer(vtkIdType valueIdx, vtkIdType numValues) VTK_OVERRIDE;
  ValueType* WritePointer(vtkIdType valueIdx, vtkIdType numValues);
  //@}

  /**
   * Removes a tuple at the given index. Default implementation
   * iterates over tuples to move elements. Subclasses are
   * encouraged to reimplemented this method to support faster implementations,
   * if needed.
   */
  void RemoveTuple(vtkIdType tupleIdx) VTK_OVERRIDE;

  /**
   * Insert data at the end of the array. Return its location in the array.
   */
  vtkIdType InsertNextValue(ValueType value);

  /**
   * Insert data at a specified position in the array.
   */
  void InsertValue(vtkIdType valueIdx, ValueType value);

  /**
   * Insert (memory allocation performed) the tuple t at tupleIdx.
   */
  void InsertTypedTuple(vtkIdType tupleIdx, const ValueType *t);

  /**
   * Insert (memory allocation performed) the tuple onto the end of the array.
   */
  vtkIdType InsertNextTypedTuple(const ValueType *t);

  /**
   * Insert (memory allocation performed) the value at the specified tuple and
   * component location.
   */
  void InsertTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType val);

  //@{
  /**
   * Get the range of array values for the given component in the
   * native data type.
   */
  void GetValueRange(ValueType range[2], int comp);
  ValueType *GetValueRange(int comp);
  //@}

  /**
   * Get the range of array values for the 0th component in the
   * native data type.
   */
  ValueType *GetValueRange() { return this->GetValueRange(0); }
  void GetValueRange(ValueType range[2]) { this->GetValueRange(range, 0); }

  /**
   * Return the capacity in typeof T units of the current array.
   * TODO Leftover from vtkDataArrayTemplate, redundant with GetSize. Deprecate?
   */
  vtkIdType Capacity() { return this->Size; }

  int GetDataType() VTK_OVERRIDE;
  int GetDataTypeSize() VTK_OVERRIDE;
  bool HasStandardMemoryLayout() VTK_OVERRIDE;
  int Allocate(vtkIdType size, vtkIdType ext = 1000) VTK_OVERRIDE;
  int Resize(vtkIdType numTuples) VTK_OVERRIDE;
  void SetNumberOfComponents(int num) VTK_OVERRIDE;
  void SetNumberOfTuples(vtkIdType number) VTK_OVERRIDE;
  void Initialize() VTK_OVERRIDE;
  void Squeeze() VTK_OVERRIDE;
  void SetTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                vtkAbstractArray* source) VTK_OVERRIDE;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::SetTuple;
  void SetTuple(vtkIdType tupleIdx, const float *tuple) VTK_OVERRIDE
  { this->Superclass::SetTuple(tupleIdx, tuple); }
  void SetTuple(vtkIdType tupleIdx, const double *tuple) VTK_OVERRIDE
  { this->Superclass::SetTuple(tupleIdx, tuple); }

  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source) VTK_OVERRIDE;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::InsertTuples;
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray* source) VTK_OVERRIDE
  { this->Superclass::InsertTuples(dstStart, n, srcStart, source); }

  void InsertTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                   vtkAbstractArray *source) VTK_OVERRIDE;
  void InsertTuple(vtkIdType tupleIdx, const float *source) VTK_OVERRIDE;
  void InsertTuple(vtkIdType tupleIdx, const double *source) VTK_OVERRIDE;
  void InsertComponent(vtkIdType tupleIdx, int compIdx,
                       double value) VTK_OVERRIDE;
  vtkIdType InsertNextTuple(vtkIdType srcTupleIdx,
                            vtkAbstractArray *source) VTK_OVERRIDE;
  vtkIdType InsertNextTuple(const float *tuple) VTK_OVERRIDE;
  vtkIdType InsertNextTuple(const double *tuple) VTK_OVERRIDE;
  void GetTuples(vtkIdList *tupleIds,
                 vtkAbstractArray *output) VTK_OVERRIDE;
  void GetTuples(vtkIdType p1, vtkIdType p2,
                 vtkAbstractArray *output) VTK_OVERRIDE;
  double *GetTuple(vtkIdType tupleIdx) VTK_OVERRIDE;
  void GetTuple(vtkIdType tupleIdx, double * tuple) VTK_OVERRIDE;
  void InterpolateTuple(vtkIdType dstTupleIdx, vtkIdList *ptIndices,
                        vtkAbstractArray* source,
                        double* weights) VTK_OVERRIDE;
  void InterpolateTuple(vtkIdType dstTupleIdx,
    vtkIdType srcTupleIdx1, vtkAbstractArray* source1,
    vtkIdType srcTupleIdx2, vtkAbstractArray* source2, double t) VTK_OVERRIDE;
  void SetComponent(vtkIdType tupleIdx, int compIdx, double value) VTK_OVERRIDE;
  double GetComponent(vtkIdType tupleIdx, int compIdx) VTK_OVERRIDE;
  void SetVariantValue(vtkIdType valueIdx, vtkVariant value) VTK_OVERRIDE;
  vtkVariant GetVariantValue(vtkIdType valueIdx) VTK_OVERRIDE;
  void InsertVariantValue(vtkIdType valueIdx, vtkVariant value) VTK_OVERRIDE;
  vtkIdType LookupValue(vtkVariant value) VTK_OVERRIDE;
  virtual vtkIdType LookupTypedValue(ValueType value);
  void LookupValue(vtkVariant value, vtkIdList* valueIds) VTK_OVERRIDE;
  virtual void LookupTypedValue(ValueType value, vtkIdList* valueIds);
  void ClearLookup() VTK_OVERRIDE;
  void DataChanged() VTK_OVERRIDE;
  VTK_NEWINSTANCE vtkArrayIterator* NewIterator() VTK_OVERRIDE;

protected:
  vtkGenericDataArray();
  ~vtkGenericDataArray() VTK_OVERRIDE;

  /**
   * Allocate space for numTuples. Old data is not preserved. If numTuples == 0,
   * all data is freed.
   * @ingroup vtkGDAConceptMethods
   */
  inline bool AllocateTuples(vtkIdType numTuples)
  {
    return static_cast<DerivedT*>(this)->AllocateTuples(numTuples);
  }

  /**
   * Allocate space for numTuples. Old data is preserved. If numTuples == 0,
   * all data is freed.
   * @ingroup vtkGDAConceptMethods
   */
  inline bool ReallocateTuples(vtkIdType numTuples)
  {
    return static_cast<DerivedT*>(this)->ReallocateTuples(numTuples);
  }

  // This method resizes the array if needed so that the given tuple index is
  // valid/accessible.
  bool EnsureAccessToTuple(vtkIdType tupleIdx);

  vtkGenericDataArrayLookupHelper<SelfType> Lookup;
private:
  vtkGenericDataArray(const vtkGenericDataArray&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGenericDataArray&) VTK_DELETE_FUNCTION;

  std::vector<double> LegacyTuple;
  std::vector<ValueType> LegacyValueRange;
};

#include "vtkGenericDataArray.txx"

// Adds an implementation of NewInstanceInternal() that returns an AoS
// (unmapped) VTK array, if possible. This allows the pipeline to copy and
// propagate the array when the array data is not modifiable. Use this in
// combination with vtkAbstractTypeMacro or vtkAbstractTemplateTypeMacro
// (instead of vtkTypeMacro) to avoid adding the default NewInstance
// implementation.
#define vtkAOSArrayNewInstanceMacro(thisClass) \
  protected: \
  vtkObjectBase *NewInstanceInternal() const VTK_OVERRIDE \
  { \
    if (vtkDataArray *da = \
        vtkDataArray::CreateDataArray(thisClass::VTK_DATA_TYPE)) \
    { \
      return da; \
    } \
    return thisClass::New(); \
  } \
  public:

#endif
// VTK-HeaderTest-Exclude: vtkGenericDataArray.h
