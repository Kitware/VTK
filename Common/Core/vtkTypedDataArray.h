// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTypedDataArray
 * @brief   Extend vtkDataArray with abstract type-specific API
 *
 *
 * This templated class decorates vtkDataArray with additional type-specific
 * methods that can be used to interact with the data.
 *
 * NOTE: This class has been made obsolete by the newer vtkGenericDataArray.
 *
 * @warning
 * This class uses vtkTypeTraits to implement GetDataType(). Since vtkIdType
 * is a typedef for either a 32- or 64-bit integer, subclasses that are designed
 * to hold vtkIdTypes will, by default, return an incorrect value from
 * GetDataType(). To fix this, such subclasses should override GetDataType() to
 * return VTK_ID_TYPE.
 *
 * @sa
 * vtkGenericDataArray
 */

#ifndef vtkTypedDataArray_h
#define vtkTypedDataArray_h

#include "vtkGenericDataArray.h"

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDeprecation.h"      // For VTK_DEPRECATED_IN_9_5_0
#include "vtkTypeTraits.h"       // For type metadata

VTK_ABI_NAMESPACE_BEGIN
template <class Scalar>
class vtkTypedDataArrayIterator;

template <class Scalar>
class VTK_DEPRECATED_IN_9_5_0(
  "This class has been made obsolete by the newer vtkGenericDataArray.") vtkTypedDataArray
  : public vtkGenericDataArray<vtkTypedDataArray<Scalar>, Scalar>
{
  typedef vtkGenericDataArray<vtkTypedDataArray<Scalar>, Scalar> GenericDataArrayType;

public:
  vtkTemplateTypeMacro(vtkTypedDataArray<Scalar>, GenericDataArrayType);
  typedef typename Superclass::ValueType ValueType;

  /**
   * Typedef to a suitable iterator class.
   */
  typedef vtkTypedDataArrayIterator<ValueType> Iterator;

  /**
   * Return an iterator initialized to the first element of the data.
   */
  Iterator Begin();

  /**
   * Return an iterator initialized to first element past the end of the data.
   */
  Iterator End();

  /**
   * Compile time access to the VTK type identifier.
   */
  enum
  {
    VTK_DATA_TYPE = vtkTypeTraits<ValueType>::VTK_TYPE_ID
  };

  /**
   * Perform a fast, safe cast from a vtkAbstractArray to a vtkTypedDataArray.
   * This method checks if:
   * - source->GetArrayType() is appropriate, and
   * - source->GetDataType() matches the Scalar template argument
   * if these conditions are met, the method performs a static_cast to return
   * source as a vtkTypedDataArray pointer. Otherwise, nullptr is returned.
   */
  static vtkTypedDataArray<Scalar>* FastDownCast(vtkAbstractArray* source);

  /**
   * Return the VTK data type held by this array.
   */
  int GetDataType() const override;

  /**
   * Return the size of the element type in bytes.
   */
  int GetDataTypeSize() const override;

  /**
   * Set the tuple value at the ith location in the array.
   */
  virtual void SetTypedTuple(vtkIdType i, const ValueType* t) = 0;

  /**
   * Insert (memory allocation performed) the tuple into the ith location
   * in the array.
   */
  virtual void InsertTypedTuple(vtkIdType i, const ValueType* t) = 0;

  /**
   * Insert (memory allocation performed) the tuple onto the end of the array.
   */
  virtual vtkIdType InsertNextTypedTuple(const ValueType* t) = 0;

  /**
   * Get the data at a particular index.
   */
  virtual ValueType GetValue(vtkIdType idx) const = 0;

  /**
   * Get a reference to the scalar value at a particular index.
   */
  virtual ValueType& GetValueReference(vtkIdType idx) = 0;

  /**
   * Set the data at a particular index. Does not do range checking. Make sure
   * you use the method SetNumberOfValues() before inserting data.
   */
  virtual void SetValue(vtkIdType idx, ValueType value) = 0;

  /**
   * Copy the tuple value into a user-provided array.
   */
  virtual void GetTypedTuple(vtkIdType idx, ValueType* t) const = 0;

  /**
   * Insert data at the end of the array. Return its location in the array.
   */
  virtual vtkIdType InsertNextValue(ValueType v) = 0;

  /**
   * Insert data at a specified position in the array.
   */
  virtual void InsertValue(vtkIdType idx, ValueType v) = 0;

  virtual ValueType GetTypedComponent(vtkIdType tupleIdx, int comp) const;
  virtual void SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType v);

  /**
   * Method for type-checking in FastDownCast implementations.
   */
  int GetArrayType() const override { return vtkAbstractArray::TypedDataArray; }

  // Reintroduced as pure virtual since the base vtkGenericDataArray method
  // requires new allocation/resize APIs, though existing MappedDataArrays
  // would just use the vtkDataArray-level virtuals.
  vtkTypeBool Allocate(vtkIdType size, vtkIdType ext = 1000) override = 0;
  vtkTypeBool Resize(vtkIdType numTuples) override = 0;

protected:
  vtkTypedDataArray();
  ~vtkTypedDataArray() override;

  /**
   * Needed for vtkGenericDataArray API, but just aborts. Override Allocate
   * instead.
   */
  virtual bool AllocateTuples(vtkIdType numTuples);

  /**
   * Needed for vtkGenericDataArray API, but just aborts. Override Resize
   * instead.
   */
  virtual bool ReallocateTuples(vtkIdType numTuples);

private:
  vtkTypedDataArray(const vtkTypedDataArray&) = delete;
  void operator=(const vtkTypedDataArray&) = delete;

  friend class vtkGenericDataArray<vtkTypedDataArray<Scalar>, Scalar>;
};

// Declare vtkArrayDownCast implementations for typed containers:
vtkArrayDownCast_TemplateFastCastMacro(vtkTypedDataArray);

// Included here to resolve chicken/egg issue with container/iterator:
VTK_ABI_NAMESPACE_END
#include "vtkTypedDataArrayIterator.h" // For iterator

VTK_ABI_NAMESPACE_BEGIN
template <class Scalar>
inline typename vtkTypedDataArray<Scalar>::Iterator vtkTypedDataArray<Scalar>::Begin()
{
  return Iterator(this, 0);
}

template <class Scalar>
inline typename vtkTypedDataArray<Scalar>::Iterator vtkTypedDataArray<Scalar>::End()
{
  return Iterator(this, this->MaxId + 1);
}

VTK_ABI_NAMESPACE_END
#include "vtkTypedDataArray.txx"

#endif // vtkTypedDataArray_h

#ifdef VTK_TYPED_VALUERANGE_INSTANTIATING

// Needed to export for this module and not CommonCore
#define VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(ArrayType, ValueType)                                 \
  template VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(                                         \
    ArrayType*, ValueType*, vtkDataArrayPrivate::AllValues, const unsigned char*, unsigned char);  \
  template VTKCOMMONCORE_EXPORT bool DoComputeScalarRange(ArrayType*, ValueType*,                  \
    vtkDataArrayPrivate::FiniteValues, const unsigned char*, unsigned char);                       \
  template VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(ArrayType*, ValueType[2],                \
    vtkDataArrayPrivate::AllValues, const unsigned char*, unsigned char);                          \
  template VTKCOMMONCORE_EXPORT bool DoComputeVectorRange(ArrayType*, ValueType[2],                \
    vtkDataArrayPrivate::FiniteValues, const unsigned char*, unsigned char);

#define VTK_INSTANTIATE_VALUERANGE_VALUETYPE(ValueType)                                            \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkTypedDataArray<ValueType>, double)

#elif defined(VTK_USE_EXTERN_TEMPLATE) // VTK_IMPLICIT_VALUERANGE_INSTANTIATING

#ifndef VTK_IMPLICIT_TEMPLATE_EXTERN
#define VTK_IMPLICIT_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the following is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif

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

#define VTK_DECLARE_VALUERANGE_VALUETYPE(ValueType)                                                \
  VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkTypedDataArray<ValueType>, double)

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
VTK_DECLARE_VALUERANGE_VALUETYPE(char)
VTK_DECLARE_VALUERANGE_VALUETYPE(signed char)
VTK_DECLARE_VALUERANGE_VALUETYPE(short)
VTK_DECLARE_VALUERANGE_VALUETYPE(int)
VTK_DECLARE_VALUERANGE_VALUETYPE(long)
VTK_DECLARE_VALUERANGE_VALUETYPE(long long)
VTK_DECLARE_VALUERANGE_VALUETYPE(unsigned char)
VTK_DECLARE_VALUERANGE_VALUETYPE(unsigned short)
VTK_DECLARE_VALUERANGE_VALUETYPE(unsigned int)
VTK_DECLARE_VALUERANGE_VALUETYPE(unsigned long)
VTK_DECLARE_VALUERANGE_VALUETYPE(unsigned long long)
VTK_DECLARE_VALUERANGE_VALUETYPE(float)
VTK_DECLARE_VALUERANGE_VALUETYPE(double)
VTK_ABI_NAMESPACE_END
}

#undef VTK_DECLARE_VALUERANGE_VALUETYPE
#undef VTK_DECLARE_VALUERANGE_ARRAYTYPE

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_IMPLICIT_TEMPLATE_EXTERN

#endif // VTK_TYPED_VALUERANGE_INSTANTIATING

// VTK-HeaderTest-Exclude: vtkTypedDataArray.h
