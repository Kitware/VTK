/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAOSDataArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAOSDataArrayTemplate
 * @brief   Array-Of-Structs implementation of
 * vtkGenericDataArray.
 *
 *
 * vtkGenericDataArray specialization that stores data array in the traditional
 * VTK memory layout where a 3 component is stored in contiguous memory as
 * \c A1A2A3B1B2B3C1C2C3 ... where A,B,C,... are tuples.
 *
 * This replaces vtkDataArrayTemplate.
 *
 * @sa
 * vtkGenericDataArray vtkSOADataArrayTemplate
*/

#ifndef vtkAOSDataArrayTemplate_h
#define vtkAOSDataArrayTemplate_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkGenericDataArray.h"
#include "vtkBuffer.h" // For storage buffer.

// The export macro below makes no sense, but is necessary for older compilers
// when we export instantiations of this class from vtkCommonCore.
template <class ValueTypeT>
class VTKCOMMONCORE_EXPORT vtkAOSDataArrayTemplate :
    public vtkGenericDataArray<vtkAOSDataArrayTemplate<ValueTypeT>, ValueTypeT>
{
  typedef vtkGenericDataArray<vtkAOSDataArrayTemplate<ValueTypeT>, ValueTypeT>
          GenericDataArrayType;
public:
  typedef vtkAOSDataArrayTemplate<ValueTypeT> SelfType;
  vtkTemplateTypeMacro(SelfType, GenericDataArrayType)
  typedef typename Superclass::ValueType ValueType;

  enum DeleteMethod
  {
    VTK_DATA_ARRAY_FREE=vtkAbstractArray::VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE=vtkAbstractArray::VTK_DATA_ARRAY_DELETE,
    VTK_DATA_ARRAY_ALIGNED_FREE=vtkAbstractArray::VTK_DATA_ARRAY_ALIGNED_FREE,
    VTK_DATA_ARRAY_USER_DEFINED=vtkAbstractArray::VTK_DATA_ARRAY_USER_DEFINED
  };

  static vtkAOSDataArrayTemplate* New();

  /**
   * Get the value at @a valueIdx. @a valueIdx assumes AOS ordering.
   */
  ValueType GetValue(vtkIdType valueIdx) const
    VTK_EXPECTS(0 <= valueIdx && valueIdx < GetNumberOfValues())
  {
    return this->Buffer->GetBuffer()[valueIdx];
  }

  /**
   * Set the value at @a valueIdx to @a value. @a valueIdx assumes AOS ordering.
   */
  void SetValue(vtkIdType valueIdx, ValueType value)
    VTK_EXPECTS(0 <= valueIdx && valueIdx < GetNumberOfValues())
  {
    this->Buffer->GetBuffer()[valueIdx] = value;
  }

  //@{
  /**
   * Copy the tuple at @a tupleIdx into @a tuple.
   */
  void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
  {
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
    std::copy(this->Buffer->GetBuffer() + valueIdx,
              this->Buffer->GetBuffer() + valueIdx + this->NumberOfComponents,
              tuple);
  }
  //@}

  //@{
  /**
   * Set this array's tuple at @a tupleIdx to the values in @a tuple.
   */
  void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
  {
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
    std::copy(tuple, tuple + this->NumberOfComponents,
              this->Buffer->GetBuffer() + valueIdx);
  }
  //@}

  /**
   * Get component @a comp of the tuple at @a tupleIdx.
   */
  ValueType GetTypedComponent(vtkIdType tupleIdx, int comp) const
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
    VTK_EXPECTS(0 <= comp && comp < GetNumberOfComponents())
  {
    return this->Buffer->GetBuffer()[this->NumberOfComponents*tupleIdx + comp];
  }

  //@{
  /**
   * Set component @a comp of the tuple at @a tupleIdx to @a value.
   */
  void SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType value)
    VTK_EXPECTS(0 <= tupleIdx && tupleIdx < GetNumberOfTuples())
    VTK_EXPECTS(0 <= comp && comp < GetNumberOfComponents())
  {
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents + comp;
    this->SetValue(valueIdx, value);
  }
  //@}

  //@{
  /**
   * Set component @a comp of all tuples to @a value.
   */
  void FillTypedComponent(int compIdx, ValueType value) override;
  //@}

  //@{
  /**
   * Set all the values in array to @a value.
   */
  void FillValue(ValueType value) override;
  void Fill(double value) override;
  //@}

  //@{
  /**
   * Get the address of a particular data index. Make sure data is allocated
   * for the number of items requested. Set MaxId according to the number of
   * data values requested.
   */
  ValueType* WritePointer(vtkIdType valueIdx, vtkIdType numValues);
  void* WriteVoidPointer(vtkIdType valueIdx, vtkIdType numValues) override;
  //@}

  //@{
  /**
   * Get the address of a particular data index. Performs no checks
   * to verify that the memory has been allocated etc.
   * Use of this method is discouraged, as newer arrays require a deep-copy of
   * the array data in order to return a suitable pointer. See vtkArrayDispatch
   * for a safer alternative for fast data access.
   */
  ValueType* GetPointer(vtkIdType valueIdx);
  void* GetVoidPointer(vtkIdType valueIdx) override;
  //@}

  //@{
  /**
   * This method lets the user specify data to be held by the array.  The
   * array argument is a pointer to the data.  size is the size of the
   * array supplied by the user (as number of values, not in bytes).
   * Set save to 1 to prevent the class from
   * deleting the array when it cleans up or reallocates memory.  The class
   * uses the actual array provided; it does not copy the data from the
   * suppled array. If specified, the delete method determines how the data
   * array will be deallocated. If the delete method is
   * VTK_DATA_ARRAY_FREE, free() will be used. If the delete method is
   * VTK_DATA_ARRAY_DELETE, delete[] will be used. If the delete method is
   * VTK_DATA_ARRAY_ALIGNED_FREE _aligned_free() will be used on Windows, while
   * free() will be used everywhere else. The default is FREE.
   */
  void SetArray(VTK_ZEROCOPY ValueType* array, vtkIdType size, int save,
                int deleteMethod);
  void SetArray(VTK_ZEROCOPY ValueType* array, vtkIdType size, int save);
  void SetVoidArray(void* array, vtkIdType size, int save) override;
  void SetVoidArray(void* array, vtkIdType size, int save,
                    int deleteMethod) override;
  //@}

  /**
    * This method allows the user to specify a custom free function to be
    * called when the array is deallocated. Calling this method will implicitly
    * mean that the given free function will be called when the class
    * cleans up or reallocates memory.
  **/
  void SetArrayFreeFunction(void (*callback)(void *)) override;

  // Overridden for optimized implementations:
  void SetTuple(vtkIdType tupleIdx, const float *tuple) override;
  void SetTuple(vtkIdType tupleIdx, const double *tuple) override;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::SetTuple;
  void SetTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                vtkAbstractArray *source) override
  { this->Superclass::SetTuple(dstTupleIdx, srcTupleIdx, source); }
  void InsertTuple(vtkIdType tupleIdx, const float *source) override;
  void InsertTuple(vtkIdType tupleIdx, const double *source) override;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::InsertTuple;
  void InsertTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx,
                   vtkAbstractArray *source) override
  { this->Superclass::InsertTuple(dstTupleIdx, srcTupleIdx, source); }
  void InsertComponent(vtkIdType tupleIdx, int compIdx,
                       double value) override;
  vtkIdType InsertNextTuple(const float *tuple) override;
  vtkIdType InsertNextTuple(const double *tuple) override;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::InsertNextTuple;
  vtkIdType InsertNextTuple(vtkIdType srcTupleIdx,
                            vtkAbstractArray *source) override
  { return this->Superclass::InsertNextTuple(srcTupleIdx, source); }
  void GetTuple(vtkIdType tupleIdx, double * tuple) override;
  double *GetTuple(vtkIdType tupleIdx) override;

  /**
   * Tell the array explicitly that a single data element has
   * changed. Like DataChanged(), then is only necessary when you
   * modify the array contents without using the array's API.
   * @note This is a legacy method from vtkDataArrayTemplate, and is only
   * implemented for array-of-struct arrays. It currently just calls
   * DataChanged() and does nothing clever.
   * TODO this is only defined for AOS (vtkDataArrayTemplate leftover).
   * Deprecate to favor DataChanged?
   */
  void DataElementChanged(vtkIdType) { this->DataChanged(); }

  /**
   * Legacy support for array-of-structs value iteration.
   * TODO Deprecate?
   */
  typedef ValueType* Iterator;
  Iterator Begin() { return Iterator(this->GetVoidPointer(0)); }
  Iterator End() { return Iterator(this->GetVoidPointer(this->MaxId + 1)); }

  //@{
  /**
   * Perform a fast, safe cast from a vtkAbstractArray to a
   * vtkAOSDataArrayTemplate.
   * This method checks if source->GetArrayType() returns DataArray
   * or a more derived type, and performs a static_cast to return
   * source as a vtkDataArray pointer. Otherwise, nullptr is returned.
   */
  static vtkAOSDataArrayTemplate<ValueType>*
  FastDownCast(vtkAbstractArray *source)
  {
    if (source)
    {
      switch (source->GetArrayType())
      {
        case vtkAbstractArray::AoSDataArrayTemplate:
          if (vtkDataTypesCompare(source->GetDataType(),
                                  vtkTypeTraits<ValueType>::VTK_TYPE_ID))
          {
            return static_cast<vtkAOSDataArrayTemplate<ValueType>*>(source);
          }
          break;
      }
    }
    return nullptr;
  }
  //@}

  int GetArrayType() override { return vtkAbstractArray::AoSDataArrayTemplate; }
  VTK_NEWINSTANCE vtkArrayIterator *NewIterator() override;
  bool HasStandardMemoryLayout() override { return true; }
  void ShallowCopy(vtkDataArray *other) override;

  // Reimplemented for efficiency:
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray* source) override;
  // MSVC doesn't like 'using' here (error C2487). Just forward instead:
  // using Superclass::InsertTuples;
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source) override
  { this->Superclass::InsertTuples(dstIds, srcIds, source); }

protected:
  vtkAOSDataArrayTemplate();
  ~vtkAOSDataArrayTemplate() override;

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

  vtkBuffer<ValueType> *Buffer;

private:
  vtkAOSDataArrayTemplate(const vtkAOSDataArrayTemplate&) = delete;
  void operator=(const vtkAOSDataArrayTemplate&) = delete;

  friend class vtkGenericDataArray<vtkAOSDataArrayTemplate<ValueTypeT>,
                                   ValueTypeT>;

};

// Declare vtkArrayDownCast implementations for AoS containers:
vtkArrayDownCast_TemplateFastCastMacro(vtkAOSDataArrayTemplate)

// This macro is used by the subclasses to create dummy
// declarations for these functions such that the wrapper
// can see them. The wrappers ignore vtkAOSDataArrayTemplate.
#define vtkCreateWrappedArrayInterface(T) \
  int GetDataType() override;\
  void GetTypedTuple(vtkIdType i, T* tuple) \
    VTK_EXPECTS(0 <= i && i < GetNumberOfTuples()); \
  void SetTypedTuple(vtkIdType i, const T* tuple) \
    VTK_EXPECTS(0 <= i && i < GetNumberOfTuples()); \
  void InsertTypedTuple(vtkIdType i, const T* tuple) \
    VTK_EXPECTS(0 <= i); \
  vtkIdType InsertNextTypedTuple(const T* tuple); \
  T GetValue(vtkIdType id) \
    VTK_EXPECTS(0 <= id && id < GetNumberOfValues()); \
  void SetValue(vtkIdType id, T value) \
    VTK_EXPECTS(0 <= id && id < GetNumberOfValues()); \
  void SetNumberOfValues(vtkIdType number) override;\
  void InsertValue(vtkIdType id, T f) \
    VTK_EXPECTS(0 <= id); \
  vtkIdType InsertNextValue(T f); \
  T *GetValueRange(int comp) VTK_SIZEHINT(2); \
  T *GetValueRange() VTK_SIZEHINT(2); \
  T* WritePointer(vtkIdType id, vtkIdType number); \
  T* GetPointer(vtkIdType id); \
  void SetArray(VTK_ZEROCOPY T* array, vtkIdType size, int save); \
  void SetArray(VTK_ZEROCOPY T* array, vtkIdType size, int save, int deleteMethod)

#endif // header guard

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// vtkAOSDataArrayTemplate can be found externally. This prevents each library
// from instantiating these on their own.
#ifdef VTK_AOS_DATA_ARRAY_TEMPLATE_INSTANTIATING
#define VTK_AOS_DATA_ARRAY_TEMPLATE_INSTANTIATE(T) \
  template class VTKCOMMONCORE_EXPORT vtkAOSDataArrayTemplate< T >
#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_AOS_DATA_ARRAY_TEMPLATE_EXTERN
#define VTK_AOS_DATA_ARRAY_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning (push)
// The following is needed when the vtkAOSDataArrayTemplate is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning (disable: 4910) // extern and dllexport incompatible
#endif
vtkExternTemplateMacro(
  extern template class VTKCOMMONCORE_EXPORT vtkAOSDataArrayTemplate)
#ifdef _MSC_VER
#pragma warning (pop)
#endif
#endif // VTK_AOS_DATA_ARRAY_TEMPLATE_EXTERN

// The following clause is only for MSVC
#elif defined(_MSC_VER) && !defined(VTK_BUILD_SHARED_LIBS)
#pragma warning (push)

// C4091: 'extern ' : ignored on left of 'int' when no variable is declared
#pragma warning (disable: 4091)

// Compiler-specific extension warning.
#pragma warning (disable: 4231)

// We need to disable warning 4910 and do an extern dllexport
// anyway.  When deriving vtkCharArray and other types from an
// instantiation of this template the compiler does an explicit
// instantiation of the base class.  From outside the vtkCommon
// library we block this using an extern dllimport instantiation.
// For classes inside vtkCommon we should be able to just do an
// extern instantiation, but VS complains about missing
// definitions.  We cannot do an extern dllimport inside vtkCommon
// since the symbols are local to the dll.  An extern dllexport
// seems to be the only way to convince VS to do the right
// thing, so we just disable the warning.
#pragma warning (disable: 4910) // extern and dllexport incompatible

// Use an "extern explicit instantiation" to give the class a DLL
// interface.  This is a compiler-specific extension.
vtkInstantiateTemplateMacro(
  extern template class VTKCOMMONCORE_EXPORT vtkAOSDataArrayTemplate)

#pragma warning (pop)

#endif

// VTK-HeaderTest-Exclude: vtkAOSDataArrayTemplate.h
