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

#include "vtkDataArray.h"
#include "vtkSmartPointer.h"
#include "vtkTypeTemplate.h"
#include "vtkTypeTraits.h"
#include "vtkAgnosticArrayLookupHelper.h"
#include "vtkAgnosticArrayHelpers.h"
#include "vtkAgnosticArrayTupleIterator.h"

#include <cassert>

template<class DerivedT,
         class ScalarTypeT,
         class TupleTypeT,
         class TupleIteratorTypeT=vtkAgnosticArrayTupleIterator<DerivedT>,
         class ScalarReturnTypeT=ScalarTypeT&>
class vtkAgnosticArray : public vtkTypeTemplate<
                         vtkAgnosticArray<DerivedT, ScalarTypeT, TupleTypeT, TupleIteratorTypeT, ScalarReturnTypeT>,
                         vtkDataArray>
{
  typedef
    vtkAgnosticArray<DerivedT, ScalarTypeT, TupleTypeT, TupleIteratorTypeT, ScalarReturnTypeT> SelfType;
public:
  typedef ScalarTypeT           ScalarType;
  typedef ScalarReturnTypeT     ScalarReturnType;
  typedef TupleTypeT            TupleType;
  typedef TupleIteratorTypeT    TupleIteratorType;

  inline TupleIteratorType Begin(vtkIdType pos=0) const
    { return TupleIteratorType(*static_cast<const DerivedT*>(this), pos); }
  inline TupleIteratorType End() const
    { return this->Begin(const_cast<SelfType*>(this)->GetNumberOfTuples()); }

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
  // Pointer access methods.
  // These are considered legacy and are not implemented. New arrays types keep
  // on supporting filters that use this API should override these methods to
  // provide appropriate implementations.

  // Description:
  // Default implementation raises a runtime error. If subclasses are keep on
  // supporting this API, they should override this method.
  virtual void *GetVoidPointer(vtkIdType id);

  // Description:
  // Implementation provided simply raises a runtime error. If subclasses are
  // keep on supporting this API, they should override the method.
  virtual void SetVoidArray(void*, vtkIdType, int);

  // Description:
  // Implementation provided simply raises a runtime error. If subclasses are
  // keep on supporting this API, they should override the method.
  virtual void* WriteVoidPointer(vtkIdType id, vtkIdType number);

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
    // XXX: We should change this to not release memory when shrinking, should
    // we?
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
    this->EnsureAccessToTuple(i);
    this->SetTuple(i, j, source);
    }
  virtual void InsertTuple(vtkIdType i, const float *source)
    {
    this->EnsureAccessToTuple(i);
    this->SetTuple(i, source);
    }
  virtual void InsertTuple(vtkIdType i, const double *source)
    {
    this->EnsureAccessToTuple(i);
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
  // SetTuple methods.
  virtual void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source);
  virtual void SetTuple(vtkIdType i, const float *source);
  virtual void SetTuple(vtkIdType i, const double *source);

  //----------------------------------------------------------------------------
  // GetTuple methods.
  virtual double *GetTuple(vtkIdType i);
  virtual void GetTuple(vtkIdType i, double * tuple);

  //----------------------------------------------------------------------------
  // Description:
  // Removes a tuple at the given index. Default implementation uses
  // TupleIteratorType to iterate over tuples to move elements. Subclasses are
  // encouraged to reimplemented this method to support faster implementations,
  // if needed.
  virtual void RemoveTuple(vtkIdType id);


  //----------------------------------------------------------------------------
  // Set Value methods. Note the index for all these methods is a "value" index
  // or component index assuming traditional VTK style memory layout for tuples
  // and components.
  virtual void SetVariantValue(vtkIdType idx, vtkVariant value);

  //----------------------------------------------------------------------------
  // All the lookup related methods We provide a default implementation that works
  // using the iterator. Since these methods are virtual, a subclass can override
  // these to provide faster alternatives.
  virtual vtkIdType LookupValue(vtkVariant value);
  virtual vtkIdType LookupTypedValue(ScalarType value);
  virtual void LookupValue(vtkVariant value, vtkIdList* ids);
  virtual void LookupTypedValue(ScalarType value, vtkIdList* ids);
  virtual void ClearLookup()
    {
    this->Lookup.ClearLookup();
    }

  virtual void DataChanged()
    {
    this->Lookup.ClearLookup();
    }

  //----------------------------------------------------------------------------
  // vtkArrayIterator API. This provides the generic vtkArrayIterator. Code
  // using vtkDataArray should stick to using the TupleIteratorType based
  // API when iterating over tuples as that is typically faster.
  // TODO:
  virtual vtkArrayIterator* NewIterator() { return NULL;}

protected:
  vtkAgnosticArray()
    : Lookup(*this)
    {
    }

  virtual ~vtkAgnosticArray()
    {
    }

  // This method resizes the array if needed so that the given tuple index is
  // valid/accessible.
  bool EnsureAccessToTuple(vtkIdType tuple)
    {
    if (tuple < 0) { return false; }
    vtkIdType expectedMaxId = (1 + tuple) * this->GetNumberOfComponents() - 1;
    if (this->MaxId < expectedMaxId)
      {
      return this->Resize(tuple + 1) != 0;
      }
    return true;
    }

  vtkAgnosticArrayLookupHelper<SelfType> Lookup;
private:
  vtkAgnosticArray(const vtkAgnosticArray&); // Not implemented.
  void operator=(const vtkAgnosticArray&); // Not implemented.
  std::vector<double> LegacyTuple;
};

#include "vtkAgnosticArray.txx"

#include <typeinfo>
#define vtkAgnosticArrayMacroCase(arrayT, scalarT, array, call) \
  if (typeid(*array) == typeid(arrayT<scalarT>)) \
    { \
    typedef arrayT<scalarT> ARRAY_TYPE; \
    ARRAY_TYPE* ARRAY = reinterpret_cast<ARRAY_TYPE*>(array); \
    call; \
    }

#define vtkWriteableAgnosticArrayMacro(array, call) \
  vtkAgnosticArrayMacroCase(vtkSoAArrayTemplate, float, array, call) \
  else vtkAgnosticArrayMacroCase(vtkSoAArrayTemplate, double, array, call) \
  else \
    { \
    vtkGenericWarningMacro("Unknown type " << typeid(*array).name()); \
    abort(); \
    }

#define vtkConstAgnosticArrayMacro(array, call) \
  vtkAgnosticArrayMacroCase(vtkSoAArrayTemplate, const float, array, call) \
  else vtkAgnosticArrayMacroCase(vtkSoAArrayTemplate, const double, array, call) \
  else vtkWriteableAgnosticArrayMacro(array, call)

#define vtkWriteableAgnosticArrayMacro2(array1, array2, call) \
  vtkWriteableAgnosticArrayMacro(array1, \
    typedef ARRAY_TYPE ARRAY_TYPE1; \
    ARRAY_TYPE1* ARRAY1 = ARRAY; \
    vtkWriteableAgnosticArrayMacro(array2, \
      typedef ARRAY_TYPE2 ARRAY_TYPE; \
      ARRAY_TYPE2* ARRAY2 = ARRAY; \
      call; \
    )\
  )

#define vtkAgnosticArrayMacro2(inarray, outarray, call) \
  vtkConstAgnosticArrayMacro(inarray, \
    typedef ARRAY_TYPE IN_ARRAY_TYPE; \
    IN_ARRAY_TYPE* IN_ARRAY = ARRAY; \
    vtkWriteableAgnosticArrayMacro(outarray, \
      typedef ARRAY_TYPE OUT_ARRAY_TYPE; \
      OUT_ARRAY_TYPE* OUT_ARRAY = ARRAY; \
      call; \
    ) \
  )

#endif
