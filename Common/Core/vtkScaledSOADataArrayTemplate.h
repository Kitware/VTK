// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkScaledSOADataArrayTemplate
 * @brief   Struct-Of-Arrays implementation of
 * vtkGenericDataArray with a scaling factor.
 *
 *
 * vtkScaledSOADataArrayTemplate is the counterpart of vtkSOADataArrayTemplate with a
 * scaling factor. Each component is stored in a separate array. The Scale value is
 * used to multiply the output of the stored value in the array. For example, if Scale
 * is 2 and the requested tuple value stored in memory is [1, 2, 3] then the returned tuple
 * values will actually be [2, 4, 6]. Similarly, if Scale is 2 and the tuple values
 * for SetTupleValue() is [2, 4, 6] then the stored values in memory will be
 * [1, 2, 3].
 *
 * @sa
 * vtkGenericDataArray vtkSOADataArrayTemplate
 */

#ifndef vtkScaledSOADataArrayTemplate_h
#define vtkScaledSOADataArrayTemplate_h

#include "vtkBuffer.h"
#include "vtkBuild.h"            // For VTK_BUILD_SHARED_LIBS
#include "vtkCommonCoreModule.h" // For export macro
#include "vtkCompiler.h"         // for VTK_USE_EXTERN_TEMPLATE
#include "vtkGenericDataArray.h"

// The export macro below makes no sense, but is necessary for older compilers
// when we export instantiations of this class from vtkCommonCore.
VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
class VTKCOMMONCORE_EXPORT vtkScaledSOADataArrayTemplate
  : public vtkGenericDataArray<vtkScaledSOADataArrayTemplate<ValueTypeT>, ValueTypeT,
      vtkArrayTypes::VTK_SCALED_SOA_DATA_ARRAY>
{
  using GenericDataArrayType = vtkGenericDataArray<vtkScaledSOADataArrayTemplate<ValueTypeT>,
    ValueTypeT, vtkArrayTypes::VTK_SCALED_SOA_DATA_ARRAY>;

public:
  using SelfType = vtkScaledSOADataArrayTemplate<ValueTypeT>;
  vtkTemplateTypeMacro(SelfType, GenericDataArrayType);
  using typename Superclass::ArrayTypeTag;
  using typename Superclass::DataTypeTag;
  using typename Superclass::ValueType;

  enum DeleteMethod
  {
    VTK_DATA_ARRAY_FREE = vtkAbstractArray::VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE = vtkAbstractArray::VTK_DATA_ARRAY_DELETE,
    VTK_DATA_ARRAY_ALIGNED_FREE = vtkAbstractArray::VTK_DATA_ARRAY_ALIGNED_FREE,
    VTK_DATA_ARRAY_USER_DEFINED = vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED
  };

  static vtkScaledSOADataArrayTemplate* New();

  ///@{
  /**
   * Set/Get the Scale value for the object. The default is 1.
   */
  void SetScale(ValueType scale)
  {
    if (scale != this->Scale)
    {
      if (scale == 0)
      {
        vtkErrorMacro("Cannot set Scale to 0");
      }
      else
      {
        this->Scale = scale;
        this->Modified();
      }
    }
  }
  ValueType GetScale() const { return this->Scale; }
  ///@}

  ///@{
  /**
   * Get the value at @a valueIdx. @a valueIdx assumes AOS ordering.
   */
  ValueType GetValue(vtkIdType valueIdx) const
    VTK_EXPECTS(0 <= valueIdx && valueIdx < GetNumberOfValues())
  {
    vtkIdType tupleIdx;
    int comp;
    this->GetTupleIndexFromValueIndex(valueIdx, tupleIdx, comp);
    return this->GetTypedComponent(tupleIdx, comp);
  }
  ///@}

  ///@{
  /**
   * Set the value at @a valueIdx to @a value. @a valueIdx assumes AOS ordering.
   */
  void SetValue(vtkIdType valueIdx, ValueType value)
    VTK_EXPECTS(0 <= valueIdx && valueIdx < GetNumberOfValues())
  {
    vtkIdType tupleIdx;
    int comp;
    this->GetTupleIndexFromValueIndex(valueIdx, tupleIdx, comp);
    this->SetTypedComponent(tupleIdx, comp, value);
  }
  ///@}

  /**
   * Copy the tuple at @a tupleIdx into @a tuple.
   */
  void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
  {
    for (size_t cc = 0; cc < this->Data.size(); cc++)
    {
      tuple[cc] = this->Data[cc]->GetBuffer()[tupleIdx] * this->Scale;
    }
  }

  /**
   * Set this array's tuple at @a tupleIdx to the values in @a tuple.
   */
  void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
  {
    for (size_t cc = 0; cc < this->Data.size(); ++cc)
    {
      this->Data[cc]->GetBuffer()[tupleIdx] = tuple[cc] / this->Scale;
    }
  }

  /**
   * Get component @a comp of the tuple at @a tupleIdx.
   */
  ValueType GetTypedComponent(vtkIdType tupleIdx, int comp) const
    VTK_EXPECTS(0 <= tupleIdx && GetNumberOfComponents() * tupleIdx + comp < GetNumberOfValues())
      VTK_EXPECTS(0 <= comp && comp < GetNumberOfComponents())
  {
    return this->Data[comp]->GetBuffer()[tupleIdx] * this->Scale;
  }

  /**
   * Set component @a comp of the tuple at @a tupleIdx to @a value.
   */
  void SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType value)
    VTK_EXPECTS(0 <= tupleIdx && GetNumberOfComponents() * tupleIdx + comp < GetNumberOfValues())
      VTK_EXPECTS(0 <= comp && comp < GetNumberOfComponents())
  {
    this->Data[comp]->GetBuffer()[tupleIdx] = value / this->Scale;
  }

  /**
   * Set component @a comp of all tuples to @a value.
   */
  void FillTypedComponent(int compIdx, ValueType value) override;

  /**
   * Use this API to pass externally allocated memory to this instance. Since
   * vtkScaledSOADataArrayTemplate uses separate contiguous regions for each
   * component, use this API to add arrays for each of the component.
   * \c save: When set to true, vtkScaledSOADataArrayTemplate will not release or
   * realloc the memory even when the AllocatorType is set to RESIZABLE. If
   * needed it will simply allow new memory buffers and "forget" the supplied
   * pointers. When save is set to false, this will be the \c deleteMethod
   * specified to release the array.
   * If updateMaxId is true, the array's MaxId will be updated, and assumes
   * that size is the number of tuples in the array.
   * \c size is specified in number of elements of ScalarType.
   */
  void SetArray(int comp, VTK_ZEROCOPY ValueType* array, vtkIdType size, bool updateMaxId = false,
    bool save = false, int deleteMethod = VTK_DATA_ARRAY_FREE);

  /**
   * This method allows the user to specify a custom free function to be
   * called when the array is deallocated. Calling this method will implicitly
   * mean that the given free function will be called when the class
   * cleans up or reallocates memory. This custom free function will be
   * used for all components.
   **/
  void SetArrayFreeFunction(void (*callback)(void*)) override;

  /**
   * This method allows the user to specify a custom free function to be
   * called when the array is deallocated. Calling this method will implicitly
   * mean that the given free function will be called when the class
   * cleans up or reallocates memory.
   **/
  void SetArrayFreeFunction(int comp, void (*callback)(void*));

  /**
   * Return a pointer to a contiguous block of memory containing all values for
   * a particular components (ie. a single array of the struct-of-arrays). Note
   * that this is to raw memory and no scaling of the data is done here.
   */
  ValueType* GetComponentArrayPointer(int comp);

  /**
   * Return the underlying buffer object for a particular component. This can
   * be used for zero-copy access to the component data, particularly useful
   * for Python buffer protocol support. Note that data in the buffer is not
   * scaled.
   */
#ifdef __VTK_WRAP__
  vtkAbstractBuffer* GetComponentBuffer(int comp);
#else
  vtkBuffer<ValueTypeT>* GetComponentBuffer(int comp);
#endif // __VTK_WRAP__

  /**
   * Use of this method is discouraged, it creates a deep copy of the data into
   * a contiguous AoS-ordered buffer and prints a warning.
   */
  void* GetVoidPointer(vtkIdType valueIdx) override;

  /**
   * Export a copy of the data in AoS ordering to the preallocated memory
   * buffer.
   */
  void ExportToVoidPointer(void* ptr) override;

  VTK_NEWINSTANCE vtkArrayIterator* NewIterator() override;
  void SetNumberOfComponents(int numComps) override;
  void ShallowCopy(vtkDataArray* other) override;

  // Reimplemented for efficiency:
  void InsertTuples(
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source) override;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::InsertTuples;
  void InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source) override
  {
    this->Superclass::InsertTuples(dstIds, srcIds, source);
  }
  void InsertTuplesStartingAt(
    vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* source) override
  {
    this->Superclass::InsertTuplesStartingAt(dstStart, srcIds, source);
  }

protected:
  vtkScaledSOADataArrayTemplate();
  ~vtkScaledSOADataArrayTemplate() override;

  /**
   * Allocate space for numTuples. Old data is not preserved. If numTuples == 0,
   * all data is freed.
   */
  bool AllocateTuples(vtkIdType numTuples);

  /**
   * Allocate space for numTuples. Old data is preserved. If numTuples == 0,
   * all data is freed.
   */
  bool ReallocateTuples(vtkIdType numTuples);

  std::vector<vtkBuffer<ValueType>*> Data;
  vtkBuffer<ValueType>* AoSCopy;

private:
  vtkScaledSOADataArrayTemplate(const vtkScaledSOADataArrayTemplate&) = delete;
  void operator=(const vtkScaledSOADataArrayTemplate&) = delete;

  void GetTupleIndexFromValueIndex(vtkIdType valueIdx, vtkIdType& tupleIdx, int& comp) const
  {
    tupleIdx = valueIdx / this->NumberOfComponents;
    comp = valueIdx % this->NumberOfComponents;
  }

  friend class vtkGenericDataArray<SelfType, ValueType, ArrayTypeTag::value>;
  /**
   * The value to scale the data stored in memory by.
   */
  ValueType Scale;
};

// Declare vtkArrayDownCast implementations for scaled SoA containers:
vtkArrayDownCast_TemplateFastCastMacro(vtkScaledSOADataArrayTemplate);

VTK_ABI_NAMESPACE_END

// This macro is used by the subclasses to create dummy
// declarations for these functions such that the wrapper
// can see them. The wrappers ignore vtkScaledSOADataArrayTemplate.
#define vtkCreateScaledSOAWrappedArrayInterface(T)                                                 \
  int GetDataType() const override;                                                                \
  T GetDataTypeValueMin() const;                                                                   \
  T GetDataTypeValueMax() const;                                                                   \
  void GetTypedTuple(vtkIdType i, T* tuple) VTK_EXPECTS(0 <= i && i < GetNumberOfTuples());        \
  T GetValue(vtkIdType id) const VTK_EXPECTS(0 <= id && id < GetNumberOfValues());                 \
  T* GetValueRange(int comp) VTK_SIZEHINT(2);                                                      \
  T* GetValueRange() VTK_SIZEHINT(2);                                                              \
  void SetTypedTuple(vtkIdType i, const T* tuple) VTK_EXPECTS(0 <= i && i < GetNumberOfTuples());  \
  void InsertTypedTuple(vtkIdType i, const T* tuple) VTK_EXPECTS(0 <= i);                          \
  vtkIdType InsertNextTypedTuple(const T* tuple);                                                  \
  void SetValue(vtkIdType id, T value) VTK_EXPECTS(0 <= id && id < GetNumberOfValues());           \
  bool SetNumberOfValues(vtkIdType number) override;                                               \
  void InsertValue(vtkIdType id, T f) VTK_EXPECTS(0 <= id);                                        \
  vtkIdType InsertNextValue(T f);                                                                  \
  T* GetComponentArrayPointer(int id);                                                             \
  vtkAbstractBuffer* GetComponentBuffer(int comp);                                                 \
  void SetArray(int comp, VTK_ZEROCOPY T* array, vtkIdType size, bool updateMaxId, bool save,      \
    int deleteMethod);

#endif // header guard

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// vtkScaledSOADataArrayTemplate can be found externally. This prevents each library
// from instantiating these on their own.
#ifdef VTK_SCALED_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATING
#define VTK_SCALED_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(T)                                          \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkScaledSOADataArrayTemplate<T>, double);                  \
  VTK_ABI_NAMESPACE_END                                                                            \
  }                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkScaledSOADataArrayTemplate<T>;                            \
  VTK_ABI_NAMESPACE_END

#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_SCALED_SOA_DATA_ARRAY_TEMPLATE_EXTERN
#define VTK_SCALED_SOA_DATA_ARRAY_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkScaledSOADataArrayTemplate is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkScaledSOADataArrayTemplate);
VTK_ABI_NAMESPACE_END

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
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
VTK_ABI_NAMESPACE_END
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_SCALED_SOA_DATA_ARRAY_TEMPLATE_EXTERN

// The following clause is only for MSVC 2008 and 2010
#elif defined(_MSC_VER) && !defined(VTK_BUILD_SHARED_LIBS)
#pragma warning(push)

// C4091: 'extern ' : ignored on left of 'int' when no variable is declared
#pragma warning(disable : 4091)

// Compiler-specific extension warning.
#pragma warning(disable : 4231)

// We need to disable warning 4910 and do an extern dllexport
// anyway.  When deriving new arrays from an
// instantiation of this template the compiler does an explicit
// instantiation of the base class.  From outside the vtkCommon
// library we block this using an extern dllimport instantiation.
// For classes inside vtkCommon we should be able to just do an
// extern instantiation, but VS 2008 complains about missing
// definitions.  We cannot do an extern dllimport inside vtkCommon
// since the symbols are local to the dll.  An extern dllexport
// seems to be the only way to convince VS 2008 to do the right
// thing, so we just disable the warning.
#pragma warning(disable : 4910) // extern and dllexport incompatible

// Use an "extern explicit instantiation" to give the class a DLL
// interface.  This is a compiler-specific extension.
VTK_ABI_NAMESPACE_BEGIN
vtkInstantiateTemplateMacro(
  extern template class VTKCOMMONCORE_EXPORT vtkScaledSOADataArrayTemplate);

#pragma warning(pop)

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkScaledSOADataArrayTemplate.h
