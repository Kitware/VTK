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
  void SetValue(vtkIdType valueIdx, ValueType value)
    VTK_EXPECTS(0 <= valueIdx && valueIdx < GetNumberOfValues())
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
  void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
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
  void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
  {
    static_cast<DerivedT*>(this)->SetTypedTuple(tupleIdx, tuple);
  }

  /**
   * Get component @a compIdx of the tuple at @a tupleIdx. This is typically
   * the fastest way to access array data.
   * @ingroup vtkGDAConceptMethods
   */
  ValueType GetTypedComponent(vtkIdType tupleIdx, int compIdx) const
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
    VTK_EXPECTS(0 <= compIdx && compIdx < GetNumberOfComponents())
  {
    return static_cast<const DerivedT*>(this)->GetTypedComponent(tupleIdx,
                                                                 compIdx);
  }

  /**
   * Set component @a compIdx of the tuple at @a tupleIdx to @a value. This is
   * typically the fastest way to set array data.
   * @ingroup vtkGDAConceptMethods
   */
  void SetTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType value)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
    VTK_EXPECTS(0 <= compIdx && compIdx < GetNumberOfComponents())
  {
    static_cast<DerivedT*>(this)->SetTypedComponent(tupleIdx, compIdx, value);
  }

  //@{
  /**
   * Default implementation raises a runtime error. If subclasses keep on
   * supporting this API, they should override this method.
   */
  void *GetVoidPointer(vtkIdType valueIdx) override;
  ValueType* GetPointer(vtkIdType valueIdx);
  void SetVoidArray(void*, vtkIdType, int) override;
  void SetVoidArray(void*, vtkIdType, int, int) override;
  void SetArrayFreeFunction(void (*callback)(void *)) override;
  void* WriteVoidPointer(vtkIdType valueIdx, vtkIdType numValues) override;
  ValueType* WritePointer(vtkIdType valueIdx, vtkIdType numValues);
  //@}

  /**
   * Removes a tuple at the given index. Default implementation
   * iterates over tuples to move elements. Subclasses are
   * encouraged to reimplemented this method to support faster implementations,
   * if needed.
   */
  void RemoveTuple(vtkIdType tupleIdx) override;

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
  ValueType *GetValueRange(int comp) VTK_SIZEHINT(2);
  //@}

  /**
   * Get the range of array values for the 0th component in the
   * native data type.
   */
  ValueType *GetValueRange() VTK_SIZEHINT(2) { return this->GetValueRange(0); }
  void GetValueRange(ValueType range[2]) { this->GetValueRange(range, 0); }

  /**
   * Return the capacity in typeof T units of the current array.
   * TODO Leftover from vtkDataArrayTemplate, redundant with GetSize. Deprecate?
   */
  vtkIdType Capacity() { return this->Size; }

  /**
   * Set component @a comp of all tuples to @a value.
   */
  virtual void FillTypedComponent(int compIdx, ValueType value);

  /**
   * Set all the values in array to @a value.
   */
  virtual void FillValue(ValueType value);

  int GetDataType() override;
  int GetDataTypeSize() override;
  bool HasStandardMemoryLayout() override;
  vtkTypeBool Allocate(vtkIdType size, vtkIdType ext = 1000) override;
  vtkTypeBool Resize(vtkIdType numTuples) override;
  void SetNumberOfComponents(int num) override;
  void SetNumberOfTuples(vtkIdType number) override;
  void Initialize() override;
  void Squeeze() override;
  void SetTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                vtkAbstractArray* source) override;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::SetTuple;
  void SetTuple(vtkIdType tupleIdx, const float *tuple) override
  { this->Superclass::SetTuple(tupleIdx, tuple); }
  void SetTuple(vtkIdType tupleIdx, const double *tuple) override
  { this->Superclass::SetTuple(tupleIdx, tuple); }

  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source) override;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::InsertTuples;
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray* source) override
  { this->Superclass::InsertTuples(dstStart, n, srcStart, source); }

  void InsertTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                   vtkAbstractArray *source) override;
  void InsertTuple(vtkIdType tupleIdx, const float *source) override;
  void InsertTuple(vtkIdType tupleIdx, const double *source) override;
  void InsertComponent(vtkIdType tupleIdx, int compIdx,
                       double value) override;
  vtkIdType InsertNextTuple(vtkIdType srcTupleIdx,
                            vtkAbstractArray *source) override;
  vtkIdType InsertNextTuple(const float *tuple) override;
  vtkIdType InsertNextTuple(const double *tuple) override;
  void GetTuples(vtkIdList *tupleIds,
                 vtkAbstractArray *output) override;
  void GetTuples(vtkIdType p1, vtkIdType p2,
                 vtkAbstractArray *output) override;
  double *GetTuple(vtkIdType tupleIdx) override;
  void GetTuple(vtkIdType tupleIdx, double * tuple) override;
  void InterpolateTuple(vtkIdType dstTupleIdx, vtkIdList *ptIndices,
                        vtkAbstractArray* source,
                        double* weights) override;
  void InterpolateTuple(vtkIdType dstTupleIdx,
    vtkIdType srcTupleIdx1, vtkAbstractArray* source1,
    vtkIdType srcTupleIdx2, vtkAbstractArray* source2, double t) override;
  void SetComponent(vtkIdType tupleIdx, int compIdx, double value) override;
  double GetComponent(vtkIdType tupleIdx, int compIdx) override;
  void SetVariantValue(vtkIdType valueIdx, vtkVariant value) override;
  vtkVariant GetVariantValue(vtkIdType valueIdx) override;
  void InsertVariantValue(vtkIdType valueIdx, vtkVariant value) override;
  vtkIdType LookupValue(vtkVariant value) override;
  virtual vtkIdType LookupTypedValue(ValueType value);
  void LookupValue(vtkVariant value, vtkIdList* valueIds) override;
  virtual void LookupTypedValue(ValueType value, vtkIdList* valueIds);
  void ClearLookup() override;
  void DataChanged() override;
  void FillComponent(int compIdx, double value) override;
  VTK_NEWINSTANCE vtkArrayIterator* NewIterator() override;

protected:
  vtkGenericDataArray();
  ~vtkGenericDataArray() override;

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

  std::vector<double> LegacyTuple;
  std::vector<ValueType> LegacyValueRange;

  vtkGenericDataArrayLookupHelper<SelfType> Lookup;

private:
  vtkGenericDataArray(const vtkGenericDataArray&) = delete;
  void operator=(const vtkGenericDataArray&) = delete;

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
  vtkObjectBase *NewInstanceInternal() const override \
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
