/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAgnosticArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAgnosticArray
// .SECTION Description

#ifndef vtkAgnosticArray_h
#define vtkAgnosticArray_h

#include "vtkAgnosticArrayTupleIterator.h"
#include "vtkDataArray.h"
#include "vtkSmartPointer.h"
#include "vtkTypeTemplate.h"
#include "vtkTypeTraits.h"
#include <cassert>

#include <typeinfo>
#define vtkAgnosticArrayMacro(array, call) \
  if (typeid(*array) == typeid(vtkSoAArrayTemplate<float>)) \
    { \
    typedef vtkSoAArrayTemplate<float> ARRAY_TYPE; \
    ARRAY_TYPE* ARRAY = reinterpret_cast<ARRAY_TYPE*>(array); \
    call ; \
    } \
  else \
    { \
    vtkGenericWarningMacro("Unknown type " << typeid(*array).name()); \
    abort();\
    }

template<class DerivedT,
         class ScalarTypeT,
         class TupleTypeT,
         class ScalarReturnTypeT=ScalarTypeT&>
class vtkAgnosticArray : public vtkTypeTemplate<
                         vtkAgnosticArray<DerivedT, ScalarTypeT, TupleTypeT, ScalarReturnTypeT>,
                         vtkDataArray>
{
  typedef
    vtkAgnosticArray<DerivedT, ScalarTypeT, TupleTypeT, ScalarReturnTypeT> SelfType;
public:
  typedef ScalarTypeT ScalarType;
  typedef TupleTypeT TupleType;
  typedef vtkAgnosticArrayTupleIterator<SelfType> TupleIteratorType;
  typedef ScalarReturnTypeT ScalarReturnType;

  inline TupleIteratorType Begin(vtkIdType pos=0) const
    { return TupleIteratorType(*static_cast<const DerivedT*>(this), pos); }
  inline TupleIteratorType End() const
    { return this->Begin(const_cast<SelfType*>(this)->GetNumberOfTuples()); }

  inline ScalarReturnType GetComponentFast(vtkIdType index, int comp) const
    {
    return static_cast<const DerivedT*>(this)->GetComponentFast(index, comp);
    }
  inline TupleType GetTupleFast(vtkIdType index) const
    {
    return static_cast<const DerivedT*>(this)->GetTupleFast(index);
    }

  // Provide implementations for pure virtual methods in vtkDataArray.

  //----------------------------------------------------------------------------
  // Core methods.
  virtual int GetDataType()
    {
    return vtkTypeTraits<ScalarType>::VTK_TYPE_ID;
    }
  virtual int GetDataTypeSize()
    {
    return static_cast<int>(sizeof(ScalarType));
    }

  //----------------------------------------------------------------------------
  // Currently unhandled methods.
  virtual void *GetVoidPointer(vtkIdType id)  { return NULL; }
  virtual void SetVoidArray(void*, vtkIdType, int) {}
  virtual vtkArrayIterator* NewIterator() { return NULL; }
  virtual vtkIdType LookupValue(vtkVariant value) { return -1; }
  virtual void LookupValue(vtkVariant value, vtkIdList* ids) {}
  virtual void SetVariantValue(vtkIdType idx, vtkVariant value) {}
  virtual void DataChanged() {}
  virtual void ClearLookup() {}
  virtual double *GetTuple(vtkIdType i) { return NULL; }
  virtual void GetTuple(vtkIdType i, double * tuple) { }
  virtual void RemoveTuple(vtkIdType id) {}
  virtual void* WriteVoidPointer(vtkIdType id, vtkIdType number) {return NULL;}

  //----------------------------------------------------------------------------
  // Methods relating to memory allocated for this array.

  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  virtual int Allocate(vtkIdType size, vtkIdType vtkNotUsed(ext))
    {
    DerivedT* self = static_cast<DerivedT*>(this);

    // Allocator must updated this->Size and this->MaxId properly.
    this->MaxId = -1;
    if (size > this->Size)
      {
      this->Size = 0;

      // let's keep the size an integral multiple of the number of components.
      size = size < 0? 0 : size;
      int numComps = this->GetNumberOfComponents() > 0? this->GetNumberOfComponents() : 1;
      vtkIdType numTuples = numComps * ceil(size/ static_cast<double>(numComps));
      // NOTE: if numTuples is 0, AllocateTuples is expected to release the
      // memory.
      if (self->AllocateTuples(numTuples) == false)
        {
        vtkErrorMacro("Unable to allocate " << size
                      << " elements of size " << sizeof(ScalarType)
                      << " bytes. ");
#if !defined NDEBUG
        // We're debugging, crash here preserving the stack
        abort();
#elif !defined VTK_DONT_THROW_BAD_ALLOC
        // We can throw something that has universal meaning
        throw std::bad_alloc();
#else
        // We indicate that alloc failed by return
        return 0;
#endif
        }
      this->Size = numTuples * numComps;
      }
    this->DataChanged();
    return 1;
    }
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
                    << " elements of size " << sizeof(ScalarType)
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
    this->MaxId = (this->Size - 1);
    return 1;
    }
  virtual void SetNumberOfTuples(vtkIdType number)
    {
    this->Resize(number);
    }
  virtual void Initialize()
    {
    this->Resize(0);
    }
  virtual void Squeeze()
    {
    this->Resize(this->MaxId+1);
    }

  //----------------------------------------------------------------------------
  // Insert* methods. The call the Set* equivalent methods after having resized,
  // if needed.
  virtual void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source)
    {
    this->EnsureAccess(i);
    this->SetTuple(i, j, source);
    }
  virtual void InsertTuple(vtkIdType i, const float *source)
    {
    this->EnsureAccess(i);
    this->SetTuple(i, source);
    }
  virtual void InsertTuple(vtkIdType i, const double *source)
    {
    this->EnsureAccess(i);
    this->SetTuple(i, source);
    }
  virtual vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray *source)
    {
    this->InsertTuple(this->MaxId, j, source);
    return this->MaxId;
    }
  virtual vtkIdType InsertNextTuple(const float *source)
    {
    this->InsertTuple(this->MaxId, source);
    return this->MaxId;
    }
  virtual vtkIdType InsertNextTuple(const double *source)
    {
    this->InsertTuple(this->MaxId, source);
    return this->MaxId;
    }
  virtual void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds, vtkAbstractArray *source);
  virtual void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source);

  //----------------------------------------------------------------------------
  // Set* methods.
  virtual void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source)
    {
    //vtkAgnosticArrayMacro(source,
    //  for (int cc=0, max=this->GetNumberOfComponents(); cc < max; ++cc)
    //    {
    //    this->Begin(i)[cc] = static_cast<ScalarType>(ARRAY.Begin(j)[cc]);
    //    }
    //  );
    }
  virtual void SetTuple(vtkIdType i, const float *source)
    {
    for (int cc=0, max=this->GetNumberOfComponents(); cc < max; ++cc)
      {
      this->Begin(i)[cc] = static_cast<ScalarType>(source[cc]);
      }
    }
  virtual void SetTuple(vtkIdType i, const double *source)
    {
    for (int cc=0, max=this->GetNumberOfComponents(); cc < max; ++cc)
      {
      this->Begin(i)[cc] = static_cast<ScalarType>(source[cc]);
      }
    }

  //----------------------------------------------------------------------------
  // All the lookup related methods that we'll implement eventually. We should
  // provide a default implementation that works using the iterator. Since these
  // methods are virtual, a subclass can override these to provide faster
  // alternatives.

protected:
  vtkAgnosticArray() { }
  virtual ~vtkAgnosticArray() { }

  // This method resizes the array if needed so that the given tuple index is
  // valid/accessible.
  bool EnsureAccess(vtkIdType tuple)
    {
    if (this->MaxId <= tuple)
      {
      return this->Resize(tuple + 1) != 0;
      }
    return true;
    }
private:
  vtkAgnosticArray(const vtkAgnosticArray&); // Not implemented.
  void operator=(const vtkAgnosticArray&); // Not implemented.
};

#include "vtkAgnosticArray.txx"

/*
#define vtkAgnosticArrayMacro2(array1, array2, callOriginal) \
  vtkAgnosticArrayMacro(array1, \
    typedef ARRAY_TYPE ARRAY_TYPE1; \
    ARRAY_TYPE& ARRAY1 = ARRAY; \
    vtkAgnosticArrayMacro(array2, \
      typedef ARRAY_TYPE ARRAY_TYPE2; \
      ARRAY_TYPE& ARRAY2 = ARRAY; \
      callOriginal \
    )\
  )
*/
#endif
