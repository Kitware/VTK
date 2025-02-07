// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericDataArray
 * @brief   Base interface for all typed vtkDataArray
 * subclasses.
 *
 *
 *
 * A more detailed description of this class and related tools can be found
 * [here](https://docs.vtk.org/en/latest/design_documents/array_dispatch.html)
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
 * vtkArrayDispatcher vtkDataArrayRange
 */

#ifndef vtkGenericDataArray_h
#define vtkGenericDataArray_h

#include "vtkDataArray.h"

#include "vtkCompiler.h" // for VTK_USE_EXTERN_TEMPLATE
#include "vtkGenericDataArrayLookupHelper.h"
#include "vtkSmartPointer.h"
#include "vtkTypeTraits.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
template <class DerivedT, class ValueTypeT>
class vtkGenericDataArray : public vtkDataArray
{
  typedef vtkGenericDataArray<DerivedT, ValueTypeT> SelfType;

public:
  typedef ValueTypeT ValueType;
  vtkTemplateTypeMacro(SelfType, vtkDataArray);

  /**
   * Compile time access to the VTK type identifier.
   */
  enum
  {
    VTK_DATA_TYPE = vtkTypeTraits<ValueType>::VTK_TYPE_ID
  };

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
  ValueType GetValue(vtkIdType valueIdx) const
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
    VTK_EXPECTS(0 <= tupleIdx && GetNumberOfComponents() * tupleIdx + compIdx < GetNumberOfValues())
      VTK_EXPECTS(0 <= compIdx && compIdx < GetNumberOfComponents())
  {
    return static_cast<const DerivedT*>(this)->GetTypedComponent(tupleIdx, compIdx);
  }

  /**
   * Set component @a compIdx of the tuple at @a tupleIdx to @a value. This is
   * typically the fastest way to set array data.
   * @ingroup vtkGDAConceptMethods
   */
  void SetTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType value)
    VTK_EXPECTS(0 <= tupleIdx && GetNumberOfComponents() * tupleIdx + compIdx < GetNumberOfValues())
      VTK_EXPECTS(0 <= compIdx && compIdx < GetNumberOfComponents())
  {
    static_cast<DerivedT*>(this)->SetTypedComponent(tupleIdx, compIdx, value);
  }

  ///@{
  /**
   * Default implementation raises a runtime error. If subclasses keep on
   * supporting this API, they should override this method.
   */
  void* GetVoidPointer(vtkIdType valueIdx) override;
  ValueType* GetPointer(vtkIdType valueIdx);
  void SetVoidArray(void*, vtkIdType, int) override;
  void SetVoidArray(void*, vtkIdType, int, int) override;
  void SetArrayFreeFunction(void (*callback)(void*)) override;
  void* WriteVoidPointer(vtkIdType valueIdx, vtkIdType numValues) override;
  ValueType* WritePointer(vtkIdType valueIdx, vtkIdType numValues);
  ///@}

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
  void InsertTypedTuple(vtkIdType tupleIdx, const ValueType* t);

  /**
   * Insert (memory allocation performed) the tuple onto the end of the array.
   */
  vtkIdType InsertNextTypedTuple(const ValueType* t);

  /**
   * Insert (memory allocation performed) the value at the specified tuple and
   * component location.
   */
  void InsertTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType val);

  ///@{
  /**
   * Get the range of array values for the given component in the
   * native data type.
   *
   * The optional `ghosts` array is used to skip values when computing the range.
   * Values whose associated ghost matches types from `ghostsToSkip` are skipped.
   * See `vtkDataSetAttributes` for a definition of ghosts.
   *
   * @sa
   * vtkDataSetAttributes
   */
  void GetValueRange(
    ValueType range[2], int comp, const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);
  void GetValueRange(ValueType range[2], int comp);
  ValueType* GetValueRange(int comp) VTK_SIZEHINT(2);
  ///@}

  /**
   * Get the range of array values for the 0th component in the
   * native data type.
   */
  ValueType* GetValueRange() VTK_SIZEHINT(2) { return this->GetValueRange(0); }
  void GetValueRange(ValueType range[2]) { this->GetValueRange(range, 0); }

  /**
   * These methods are analogous to the GetValueRange methods, except that the
   * only consider finite values.
   * @{
   */
  void GetFiniteValueRange(
    ValueType range[2], int comp, const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);
  void GetFiniteValueRange(ValueType range[2], int comp);
  ValueType* GetFiniteValueRange(int comp) VTK_SIZEHINT(2);
  ValueType* GetFiniteValueRange() VTK_SIZEHINT(2) { return this->GetFiniteValueRange(0); }
  void GetFiniteValueRange(ValueType range[2]) { this->GetFiniteValueRange(range, 0); }
  /**@}*/

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

  int GetDataType() const override;
  int GetDataTypeSize() const override;
  bool HasStandardMemoryLayout() const override;
  vtkTypeBool Allocate(vtkIdType size, vtkIdType ext = 1000) override;
  vtkTypeBool Resize(vtkIdType numTuples) override;
  void SetNumberOfComponents(int num) override;
  void SetNumberOfTuples(vtkIdType number) override;
  void Initialize() override;
  void Squeeze() override;
  void SetTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx, vtkAbstractArray* source) override;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::SetTuple;
  void SetTuple(vtkIdType tupleIdx, const float* tuple) override
  {
    this->Superclass::SetTuple(tupleIdx, tuple);
  }
  void SetTuple(vtkIdType tupleIdx, const double* tuple) override
  {
    this->Superclass::SetTuple(tupleIdx, tuple);
  }

  void InsertTuplesStartingAt(
    vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* source) override;
  void InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source) override;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::InsertTuples;
  void InsertTuples(
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source) override
  {
    this->Superclass::InsertTuples(dstStart, n, srcStart, source);
  }

  void InsertTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx, vtkAbstractArray* source) override;
  void InsertTuple(vtkIdType tupleIdx, const float* source) override;
  void InsertTuple(vtkIdType tupleIdx, const double* source) override;
  void InsertComponent(vtkIdType tupleIdx, int compIdx, double value) override;
  vtkIdType InsertNextTuple(vtkIdType srcTupleIdx, vtkAbstractArray* source) override;
  vtkIdType InsertNextTuple(const float* tuple) override;
  vtkIdType InsertNextTuple(const double* tuple) override;
  void GetTuples(vtkIdList* tupleIds, vtkAbstractArray* output) override;
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray* output) override;
  double* GetTuple(vtkIdType tupleIdx) override;
  void GetTuple(vtkIdType tupleIdx, double* tuple) override;
  void InterpolateTuple(vtkIdType dstTupleIdx, vtkIdList* ptIndices, vtkAbstractArray* source,
    double* weights) override;
  void InterpolateTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx1, vtkAbstractArray* source1,
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
  bool AllocateTuples(vtkIdType numTuples)
  {
    return static_cast<DerivedT*>(this)->AllocateTuples(numTuples);
  }

  /**
   * Allocate space for numTuples. Old data is preserved. If numTuples == 0,
   * all data is freed.
   * @ingroup vtkGDAConceptMethods
   */
  bool ReallocateTuples(vtkIdType numTuples)
  {
    return static_cast<DerivedT*>(this)->ReallocateTuples(numTuples);
  }

  // This method resizes the array if needed so that the given tuple index is
  // valid/accessible.
  bool EnsureAccessToTuple(vtkIdType tupleIdx);

  /**
   * Compute the range for a specific component. If comp is set -1
   * then L2 norm is computed on all components. Call ClearRange
   * to force a recomputation if it is needed. The range is copied
   * to the range argument.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void ComputeValueRange(
    ValueType range[2], int comp, const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);

  /**
   * Compute the range for a specific component. If comp is set -1
   * then L2 norm is computed on all components. Call ClearRange
   * to force a recomputation if it is needed. The range is copied
   * to the range argument.
   * THIS METHOD IS NOT THREAD SAFE.
   */
  void ComputeFiniteValueRange(
    ValueType range[2], int comp, const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);

  /**
   * Computes the range for each component of an array, the length
   * of \a ranges must be two times the number of components.
   * Returns true if the range was computed. Will return false
   * if you try to compute the range of an array of length zero.
   */
  bool ComputeScalarValueRange(
    ValueType* ranges, const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);

  /**
   * Returns true if the range was computed. Will return false
   * if you try to compute the range of an array of length zero.
   */
  bool ComputeVectorValueRange(
    ValueType range[2], const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);

  /**
   * Computes the range for each component of an array, the length
   * of \a ranges must be two times the number of components.
   * Returns true if the range was computed. Will return false
   * if you try to compute the range of an array of length zero.
   */
  bool ComputeFiniteScalarValueRange(
    ValueType* ranges, const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);

  /**
   * Returns true if the range was computed. Will return false
   * if you try to compute the range of an array of length zero.
   */
  bool ComputeFiniteVectorValueRange(
    ValueType range[2], const unsigned char* ghosts, unsigned char ghostsToSkip = 0xff);

  std::vector<double> LegacyTuple;
  std::vector<ValueType> LegacyValueRange;
  std::vector<ValueType> LegacyValueRangeFull;

  vtkGenericDataArrayLookupHelper<SelfType> Lookup;

private:
  vtkGenericDataArray(const vtkGenericDataArray&) = delete;
  void operator=(const vtkGenericDataArray&) = delete;
};
VTK_ABI_NAMESPACE_END

// these predeclarations are needed before the .txx include for MinGW
namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
template <typename A, typename R, typename T>
VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(
  A*, R*, T, const unsigned char* ghosts, unsigned char ghostsToSkip);
template <typename A, typename R>
VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(
  A*, R[2], AllValues, const unsigned char* ghosts, unsigned char ghostsToSkip);
template <typename A, typename R>
VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(
  A*, R[2], FiniteValues, const unsigned char* ghosts, unsigned char ghostsToSkip);
VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#include "vtkGenericDataArray.txx"

// Adds an implementation of NewInstanceInternal() that returns an AoS
// (unmapped) VTK array, if possible. This allows the pipeline to copy and
// propagate the array when the array data is not modifiable. Use this in
// combination with vtkAbstractTypeMacro or vtkAbstractTemplateTypeMacro
// (instead of vtkTypeMacro) to avoid adding the default NewInstance
// implementation.
#define vtkAOSArrayNewInstanceMacro(thisClass)                                                     \
protected:                                                                                         \
  vtkObjectBase* NewInstanceInternal() const override                                              \
  {                                                                                                \
    if (vtkDataArray* da = vtkDataArray::CreateDataArray(thisClass::VTK_DATA_TYPE))                \
    {                                                                                              \
      return da;                                                                                   \
    }                                                                                              \
    return thisClass::New();                                                                       \
  }                                                                                                \
                                                                                                   \
public:

#endif

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// the GetValueRange lookups can be found externally. This prevents each library
// from instantiating these on their own.
// Additionally it helps hide implementation details that pull in system
// headers.
// We only provide these specializations for the 64-bit integer types, since
// other types can reuse the double-precision mechanism in
// vtkDataArray::GetRange without losing precision.
#ifdef VTK_GDA_VALUERANGE_INSTANTIATING

// Forward declare necessary stuffs:
VTK_ABI_NAMESPACE_BEGIN
template <typename ValueType>
class vtkAOSDataArrayTemplate;
template <typename ValueType>
class vtkSOADataArrayTemplate;

#ifdef VTK_USE_SCALED_SOA_ARRAYS
template <typename ValueType>
class vtkScaledSOADataArrayTemplate;
#endif
VTK_ABI_NAMESPACE_END

#define VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(ArrayType, ValueType)                                 \
  template VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(                                         \
    ArrayType*, ValueType*, vtkDataArrayPrivate::AllValues, const unsigned char*, unsigned char);  \
  template VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(ArrayType*, ValueType*,                  \
    vtkDataArrayPrivate::FiniteValues, const unsigned char*, unsigned char);                       \
  template VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(ArrayType*, ValueType[2],                \
    vtkDataArrayPrivate::AllValues, const unsigned char*, unsigned char);                          \
  template VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(ArrayType*, ValueType[2],                \
    vtkDataArrayPrivate::FiniteValues, const unsigned char*, unsigned char);

#ifdef VTK_USE_SCALED_SOA_ARRAYS

#define VTK_INSTANTIATE_VALUERANGE_VALUETYPE(ValueType)                                            \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<ValueType>, ValueType)              \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<ValueType>, ValueType)              \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<ValueType>, ValueType)

#else // VTK_USE_SCALED_SOA_ARRAYS

#define VTK_INSTANTIATE_VALUERANGE_VALUETYPE(ValueType)                                            \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<ValueType>, ValueType)              \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<ValueType>, ValueType)

#endif

#elif defined(VTK_USE_EXTERN_TEMPLATE) // VTK_GDA_VALUERANGE_INSTANTIATING

#ifndef VTK_GDA_TEMPLATE_EXTERN
#define VTK_GDA_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the following is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif

VTK_ABI_NAMESPACE_BEGIN
// Forward declare necessary stuffs:
template <typename ValueType>
class vtkAOSDataArrayTemplate;
template <typename ValueType>
class vtkSOADataArrayTemplate;

#ifdef VTK_USE_SCALED_SOA_ARRAYS
template <typename ValueType>
class vtkScaledSOADataArrayTemplate;
#endif

VTK_ABI_NAMESPACE_END

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
template <typename A, typename R, typename T>
VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(
  A*, R*, T, const unsigned char* ghosts, unsigned char ghostsToSkip);
template <typename A, typename R>
VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(
  A*, R[2], AllValues, const unsigned char* ghosts, unsigned char ghostsToSkip);
template <typename A, typename R>
VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(
  A*, R[2], FiniteValues, const unsigned char* ghosts, unsigned char ghostsToSkip);
VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#define VTK_DECLARE_VALUERANGE_ARRAYTYPE(ArrayType, ValueType)                                     \
  extern template VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(                                  \
    ArrayType*, ValueType*, vtkDataArrayPrivate::AllValues, const unsigned char*, unsigned char);  \
  extern template VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(ArrayType*, ValueType*,           \
    vtkDataArrayPrivate::FiniteValues, const unsigned char*, unsigned char);                       \
  extern template VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(ArrayType*, ValueType[2],         \
    vtkDataArrayPrivate::AllValues, const unsigned char*, unsigned char);                          \
  extern template VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(ArrayType*, ValueType[2],         \
    vtkDataArrayPrivate::FiniteValues, const unsigned char*, unsigned char);

#ifdef VTK_USE_SCALED_SOA_ARRAYS

#define VTK_DECLARE_VALUERANGE_VALUETYPE(ValueType)                                                \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<ValueType>, ValueType)                  \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<ValueType>, ValueType)                  \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<ValueType>, ValueType)

#else // VTK_USE_SCALED_SOA_ARRAYS

#define VTK_DECLARE_VALUERANGE_VALUETYPE(ValueType)                                                \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<ValueType>, ValueType)                  \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<ValueType>, ValueType)

#endif

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
// These are instantiated in vtkGenericDataArrayValueRange${i}.cxx
VTK_DECLARE_VALUERANGE_VALUETYPE(long)
VTK_DECLARE_VALUERANGE_VALUETYPE(unsigned long)
VTK_DECLARE_VALUERANGE_VALUETYPE(long long)
VTK_DECLARE_VALUERANGE_VALUETYPE(unsigned long long)

// This is instantiated in vtkGenericDataArray.cxx
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkDataArray, double)

// These are instantiated in vtkFloatArray.cxx, vtkDoubleArray.cxx, etc
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<float>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<double>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<signed char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<unsigned char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<unsigned short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<unsigned int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<unsigned long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<long long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAOSDataArrayTemplate<unsigned long long>, double)

// These are instantiated in vtkSOADataArrayTemplateInstantiate${i}.cxx
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<float>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<double>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<signed char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<unsigned char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<unsigned short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<unsigned int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<unsigned long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<long long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkSOADataArrayTemplate<unsigned long long>, double)

// These are instantiated in vtkScaledSOADataArrayTemplateInstantiate${i}.cxx
#ifdef VTK_USE_SCALED_SOA_ARRAYS
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<float>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<double>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<signed char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<unsigned char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<unsigned short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<unsigned int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<unsigned long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<long long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<unsigned long long>, double)
#endif // VTK_USE_SCALED_SOA_ARRAYS

VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#undef VTK_DECLARE_VALUERANGE_ARRAYTYPE
#undef VTK_DECLARE_VALUERANGE_VALUETYPE

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_GDA_TEMPLATE_EXTERN

#endif // VTK_GDA_VALUERANGE_INSTANTIATING

// VTK-HeaderTest-Exclude: vtkGenericDataArray.h
