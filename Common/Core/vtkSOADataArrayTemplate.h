/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSOADataArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSOADataArrayTemplate - Struct-Of-Arrays implementation of
// vtkGenericDataArray.
//
// .SECTION Description
// vtkSOADataArrayTemplate is the counterpart of vtkAOSDataArrayTemplate. Each
// component is stored in a separate array.
//
// .SECTION See Also
// vtkGenericDataArray vtkAOSDataArrayTemplate

#ifndef vtkSOADataArrayTemplate_h
#define vtkSOADataArrayTemplate_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkGenericDataArray.h"
#include "vtkBuffer.h"

// The export macro below makes no sense, but is necessary for older compilers
// when we export instantiations of this class from vtkCommonCore.
template <class ValueTypeT>
class VTKCOMMONCORE_EXPORT vtkSOADataArrayTemplate :
    public vtkGenericDataArray<vtkSOADataArrayTemplate<ValueTypeT>, ValueTypeT>
{
  typedef vtkGenericDataArray<vtkSOADataArrayTemplate<ValueTypeT>, ValueTypeT>
          GenericDataArrayType;
public:
  typedef vtkSOADataArrayTemplate<ValueTypeT> SelfType;
  vtkTemplateTypeMacro(SelfType, GenericDataArrayType)
  typedef typename Superclass::ValueType ValueType;

  enum DeleteMethod
    {
    VTK_DATA_ARRAY_FREE=vtkBuffer<ValueType>::VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE=vtkBuffer<ValueType>::VTK_DATA_ARRAY_DELETE
    };

  static vtkSOADataArrayTemplate* New();

  // Description:
  // Get the value at @a valueIdx. @a valueIdx assumes AOS ordering.
  inline ValueType GetValue(vtkIdType valueIdx) const
  {
    vtkIdType tupleIdx;
    int comp;
    this->GetTupleIndexFromValueIndex(valueIdx, tupleIdx, comp);
    return this->GetTypedComponent(tupleIdx, comp);
  }

  // Description:
  // Set the value at @a valueIdx to @a value. @a valueIdx assumes AOS ordering.
  inline void SetValue(vtkIdType valueIdx, ValueType value)
  {
    vtkIdType tupleIdx;
    int comp;
    this->GetTupleIndexFromValueIndex(valueIdx, tupleIdx, comp);
    this->SetTypedComponent(tupleIdx, comp, value);
  }

  // Description:
  // Copy the tuple at @a tupleIdx into @a tuple.
  inline void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
  {
    for (size_t cc=0; cc < this->Data.size(); cc++)
      {
      tuple[cc] = this->Data[cc]->GetBuffer()[tupleIdx];
      }
  }

  // Description:
  // Set this array's tuple at @a tupleIdx to the values in @a tuple.
  inline void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
  {
    for (size_t cc=0; cc < this->Data.size(); ++cc)
      {
      this->Data[cc]->GetBuffer()[tupleIdx] = tuple[cc];
      }
  }

  // Description:
  // Get component @a comp of the tuple at @a tupleIdx.
  inline ValueType GetTypedComponent(vtkIdType tupleIdx, int comp) const
  {
    return this->Data[comp]->GetBuffer()[tupleIdx];
  }

  // Description:
  // Set component @a comp of the tuple at @a tupleIdx to @a value.
  inline void SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType value)
  {
    this->Data[comp]->GetBuffer()[tupleIdx] = value;
  }

  // Description:
  // Use this API to pass externally allocated memory to this instance. Since
  // vtkSOADataArrayTemplate uses separate contiguous regions for each
  // component, use this API to add arrays for each of the component.
  // \c save: When set to true, vtkSOADataArrayTemplate will not release or
  // realloc the memory even when the AllocatorType is set to RESIZABLE. If
  // needed it will simply allow new memory buffers and "forget" the supplied
  // pointers. When save is set to false, this will be the \c deleteMethod
  // specified to release the array.
  // If updateMaxId is true, the array's MaxId will be updated, and assumes
  // that size is the number of tuples in the array.
  // \c size is specified in number of elements of ScalarType.
  void SetArray(int comp, ValueType* array, vtkIdType size,
                bool updateMaxId = false, bool save=false,
                int deleteMethod=VTK_DATA_ARRAY_FREE);

  // Description:
  // Return a pointer to a contiguous block of memory containing all values for
  // a particular components (ie. a single array of the struct-of-arrays).
  ValueType* GetComponentArrayPointer(int comp);

  // Description:
  // Use of this method is discouraged, it creates a deep copy of the data into
  // a contiguous AoS-ordered buffer and prints a warning.
  virtual void *GetVoidPointer(vtkIdType valueIdx);

  // Description:
  // Export a copy of the data in AoS ordering to the preallocated memory
  // buffer.
  void ExportToVoidPointer(void *ptr);

  // Description:
  // Perform a fast, safe cast from a vtkAbstractArray to a vtkDataArray.
  // This method checks if source->GetArrayType() returns DataArray
  // or a more derived type, and performs a static_cast to return
  // source as a vtkDataArray pointer. Otherwise, NULL is returned.
  static vtkSOADataArrayTemplate<ValueType>*
  FastDownCast(vtkAbstractArray *source)
  {
    switch (source->GetArrayType())
      {
      case vtkAbstractArray::SoADataArrayTemplate:
        if (vtkDataTypesCompare(source->GetDataType(),
                                vtkTypeTraits<ValueType>::VTK_TYPE_ID))
          {
          return static_cast<vtkSOADataArrayTemplate<ValueType>*>(source);
          }
        break;
      }
    return NULL;
  }

  virtual int GetArrayType() { return vtkAbstractArray::SoADataArrayTemplate; }
  virtual vtkArrayIterator *NewIterator();
  virtual void SetNumberOfComponents(int numComps);
  virtual void ShallowCopy(vtkDataArray *other);

protected:
  vtkSOADataArrayTemplate();
  ~vtkSOADataArrayTemplate();

  // Description:
  // Allocate space for numTuples. Old data is not preserved. If numTuples == 0,
  // all data is freed.
  bool AllocateTuples(vtkIdType numTuples);

  // Description:
  // Allocate space for numTuples. Old data is preserved. If numTuples == 0,
  // all data is freed.
  bool ReallocateTuples(vtkIdType numTuples);

  std::vector<vtkBuffer<ValueType>*> Data;
  vtkBuffer<ValueType> *AoSCopy;

  double NumberOfComponentsReciprocal;

private:
  vtkSOADataArrayTemplate(const vtkSOADataArrayTemplate&); // Not implemented.
  void operator=(const vtkSOADataArrayTemplate&); // Not implemented.

  inline void GetTupleIndexFromValueIndex(vtkIdType valueIdx,
                                          vtkIdType& tupleIdx, int& comp) const
  {
    tupleIdx = static_cast<vtkIdType>(valueIdx *
                                      this->NumberOfComponentsReciprocal);
    comp = valueIdx - (tupleIdx * this->NumberOfComponents);
  }

  friend class vtkGenericDataArray<vtkSOADataArrayTemplate<ValueTypeT>,
                                   ValueTypeT>;
};

// Declare vtkArrayDownCast implementations for SoA containers:
vtkArrayDownCast_TemplateFastCastMacro(vtkSOADataArrayTemplate)

# define VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(T) \
  template class VTKCOMMONCORE_EXPORT vtkSOADataArrayTemplate< T >

#endif // header guard

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// vtkSOADataArrayTemplate can be found externally. This prevents each library
// from instantiating these on their own.
#ifndef VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATING
#if defined(VTK_BUILD_SHARED_LIBS) && defined(_MSC_VER)
#pragma warning (push)

// C4091: 'extern ' : ignored on left of 'int' when no variable is declared
#pragma warning (disable: 4091)

// Compiler-specific extension warning.
#pragma warning (disable: 4231)

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
#pragma warning (disable: 4910) // extern and dllexport incompatible

// Use an "extern explicit instantiation" to give the class a DLL
// interface.  This is a compiler-specific extension.
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(char);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(double);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(float);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(int);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(long);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(long long);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(short);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(signed char);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned char);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned int);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned long);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned long long);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned short);

#pragma warning (pop)

#endif // VTK_BUILD_SHARED_LIBS && _MSC_VER
#endif // VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATING

// VTK-HeaderTest-Exclude: vtkSOADataArrayTemplate.h
