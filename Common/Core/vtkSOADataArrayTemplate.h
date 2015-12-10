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
// .NAME vtkSOADataArrayTemplate
// .SECTION Description
// vtkSOADataArrayTemplate is the counterpart of vtkAOSDataArrayTemplate.

#ifndef vtkSOADataArrayTemplate_h
#define vtkSOADataArrayTemplate_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkGenericDataArray.h"
#include "vtkBuffer.h"

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
  typedef typename Superclass::ReferenceType ReferenceType;
  typedef typename Superclass::ConstReferenceType ConstReferenceType;

  static vtkSOADataArrayTemplate* New();

  // **************************************************************************
  // Methods that are needed to be implemented by every vtkGenericDataArray
  // subclass.
  // **************************************************************************
  inline ConstReferenceType GetValue(vtkIdType valueIdx) const
    {
    vtkIdType tupleIdx;
    int comp;
    this->GetTupleIndexFromValueIndex(valueIdx, tupleIdx, comp);
    return this->GetComponentValue(tupleIdx, comp);
    }
  inline void GetTupleValue(vtkIdType tupleIdx, ValueType* tuple) const
    {
    for (size_t cc=0; cc < this->Data.size(); cc++)
      {
      tuple[cc] = this->Data[cc].GetBuffer()[tupleIdx];
      }
    }
  inline ConstReferenceType GetComponentValue(vtkIdType tupleIdx,
                                              int comp) const
    {
    return this->Data[comp].GetBuffer()[tupleIdx];
    }
  inline void SetValue(vtkIdType valueIdx, ValueType value)
    {
    vtkIdType tupleIdx;
    int comp;
    this->GetTupleIndexFromValueIndex(valueIdx, tupleIdx, comp);
    this->SetComponentValue(tupleIdx, comp, value);
    }
  inline void SetTupleValue(vtkIdType tupleIdx, const ValueType* tuple)
    {
    for (size_t cc=0; cc < this->Data.size(); ++cc)
      {
      this->Data[cc].GetBuffer()[tupleIdx] = tuple[cc];
      }
    }
  inline void SetComponentValue(vtkIdType tupleIdx, int comp, ValueType value)
    {
    this->Data[comp].GetBuffer()[tupleIdx] = value;
    }

  // **************************************************************************

  enum DeleteMethod
    {
    VTK_DATA_ARRAY_FREE=vtkBuffer<ValueType>::VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE=vtkBuffer<ValueType>::VTK_DATA_ARRAY_DELETE
    };

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
  virtual void *GetVoidPointer(vtkIdType id);

  // Description:
  // Export a copy of the data in AoS ordering to the preallocated memory
  // buffer.
  void ExportToVoidPointer(void *ptr);

  // Description:
  // Overridden to allocate pointer for each component.
  virtual void SetNumberOfComponents(int);

  // Description:
  // Call this method before using any of the methods on this array that affect
  // memory allocation. When set to false, any attempt to grow the arrays will
  // raise runtime exceptions. Any attempt to shrink the arrays will have no
  // effect.
  vtkSetMacro(Resizeable, bool);
  vtkGetMacro(Resizeable, bool);
  vtkBooleanMacro(Resizeable, bool);

  virtual vtkArrayIterator *NewIterator();

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

  // Description:
  // Method for type-checking in FastDownCast implementations.
  virtual int GetArrayType() { return vtkAbstractArray::SoADataArrayTemplate; }

protected:
  vtkSOADataArrayTemplate();
  ~vtkSOADataArrayTemplate();

  // **************************************************************************
  // Methods that are needed to be implemented by every vtkGenericDataArray
  // subclass.
  // **************************************************************************
  // Implement the memory management interface.
  bool AllocateTuples(vtkIdType numTuples);
  bool ReallocateTuples(vtkIdType numTuples);
  // **************************************************************************

  std::vector<vtkBuffer<ValueType> > Data;
  vtkBuffer<ValueType> AoSCopy;
  bool Resizeable;
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
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(short);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(signed char);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned char);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned int);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned long);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned short);
#ifdef VTK_TYPE_USE_LONG_LONG
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(long long);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned long long);
#endif // VTK_TYPE_USE_LONG_LONG
#ifdef VTK_TYPE_USE___INT64
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(__int64);
extern VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned __int64);
#endif // VTK_TYPE_USE___INT64

#pragma warning (pop)

#endif // VTK_BUILD_SHARED_LIBS && _MSC_VER
#endif // VTK_SOA_DATA_ARRAY_TEMPLATE_INSTANTIATING

// VTK-HeaderTest-Exclude: vtkSOADataArrayTemplate.h
