/*==============================================================================

  Program:   Visualization Toolkit
  Module:    vtkMappedDataArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/
// .NAME vtkMappedDataArray - Map non-contiguous data structures into the
// vtkDataArray API.
//
// .SECTION Description
// vtkMappedDataArray is a superclass for vtkDataArrays that do not use
// the standard memory layout, and allows VTK to interface with
// simulation codes for in-situ analysis without repacking simulation data.
//
// vtkMappedDataArrayNewInstanceMacro is used by subclasses to implement
// NewInstanceInternal such that a non-mapped vtkDataArray is returned by
// NewInstance(). This prevents the mapped array type from propogating
// through the pipeline.
//
// .SECTION Notes
// Subclasses that hold vtkIdType elements must also
// reimplement `int GetDataType()` (see Caveat in vtkTypedDataArray).

#ifndef __vtkMappedDataArray_h
#define __vtkMappedDataArray_h

#include "vtkTypedDataArray.h"

#include "vtkTypeTemplate.h" // for vtkTypeTemplate

template <class Scalar>
class vtkMappedDataArray : public vtkTypeTemplate<vtkMappedDataArray<Scalar>,
                                                  vtkTypedDataArray<Scalar> >
{
public:
  typedef vtkTypedDataArray<Scalar> Superclass;
  typedef typename Superclass::ValueType ValueType;

  // Description:
  // Perform a fast, safe cast from a vtkAbstractArray to a vtkMappedDataArray.
  // This method checks if:
  // - source->GetArrayType() is appropriate, and
  // - source->GetDataType() matches the Scalar template argument
  // if these conditions are met, the method performs a static_cast to return
  // source as a vtkMappedDataArray pointer. Otherwise, NULL is returned.
  static vtkMappedDataArray<Scalar>* FastDownCast(vtkAbstractArray *source);

  void PrintSelf(ostream &os, vtkIndent indent);

  // vtkAbstractArray virtual method that must be reimplemented.
  void DeepCopy(vtkAbstractArray *aa) = 0;
  vtkVariant GetVariantValue(vtkIdType idx) = 0;
  void SetVariantValue(vtkIdType idx, vtkVariant value) = 0;
  void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output) = 0;
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output) = 0;
  void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
                        vtkAbstractArray *source, double *weights) = 0;
  void InterpolateTuple(vtkIdType i, vtkIdType id1,
                        vtkAbstractArray* source1, vtkIdType id2,
                        vtkAbstractArray* source2, double t) = 0;

  // vtkDataArray virtual method that must be reimplemented.
  void DeepCopy(vtkDataArray *da) = 0;

  // Description:
  // Print an error and create an internal, long-lived temporary array. This
  // method should not be used on vtkMappedDataArray subclasses. See
  // vtkTypedDataArrayIterator and/or vtkDataArrayIteratorMacro instead.
  void * GetVoidPointer(vtkIdType id);

  // Description:
  // Copy the internal data to the void pointer. The pointer is cast to this
  // array's Scalar type and vtkTypedDataArrayIterator is used to populate
  // the input array.
  void ExportToVoidPointer(void *ptr);

  // Description:
  // Read the data from the internal temporary array (created by GetVoidPointer)
  // back into the mapped array. If GetVoidPointer has not been called (and the
  // internal array therefore does not exist), print an error and return. The
  // default implementation uses vtkTypedDataArrayIterator to extract the mapped
  // data.
  void DataChanged();

  // Description:
  // This method doesn't make sense for mapped data array. Prints an error and
  // returns.
  void SetVoidArray(void *, vtkIdType, int);

  // Description:
  // Not implemented. Print error and return NULL.
  void * WriteVoidPointer(vtkIdType /*id*/, vtkIdType /*number*/)
  {
    vtkErrorMacro(<<"WriteVoidPointer: Method not implemented.");
    return NULL;
  }

  // Description:
  // Invalidate the internal temporary array and call superclass method.
  void Modified();

  // vtkAbstractArray override:
  bool HasStandardMemoryLayout() { return false; }

protected:
  vtkMappedDataArray();
  ~vtkMappedDataArray();

  virtual int GetArrayType()
  {
    return vtkAbstractArray::MappedDataArray;
  }

private:
  vtkMappedDataArray(const vtkMappedDataArray &); // Not implemented.
  void operator=(const vtkMappedDataArray &);   // Not implemented.

  // Description: Temporary internal array used as fall back storage for
  // GetVoidPointer.
  ValueType *TemporaryScalarPointer;
  size_t TemporaryScalarPointerSize;
};

#include "vtkMappedDataArray.txx"

// Adds an implementation of NewInstanceInternal() that returns a standard
// (unmapped) VTK array, if possible. Use this with classes that derive from
// vtkTypeTemplate, otherwise, use vtkMappedDataArrayTypeMacro.
#define vtkMappedDataArrayNewInstanceMacro(thisClass) \
  protected: \
  vtkObjectBase *NewInstanceInternal() const \
  { \
    if (vtkDataArray *da = \
        vtkDataArray::CreateDataArray(thisClass::VTK_DATA_TYPE)) \
      { \
      return da; \
      } \
    return thisClass::New(); \
  } \
  public:

// Same as vtkTypeMacro, but adds an implementation of NewInstanceInternal()
// that returns a standard (unmapped) VTK array, if possible.
#define vtkMappedDataArrayTypeMacro(thisClass, superClass) \
  vtkAbstractTypeMacroWithNewInstanceType(thisClass, superClass, vtkDataArray) \
  vtkMappedDataArrayNewInstanceMacro(thisClass)

#endif //__vtkMappedDataArray_h

// VTK-HeaderTest-Exclude: vtkMappedDataArray.h
