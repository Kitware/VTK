/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPeriodicDataArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPeriodicDataArray - Map native an Array into an angulat
// periodic array
//
// .SECTION Description
// Map an array into a periodic array. Data from the original array aare
// rotated (on the fly) by the specified angle along the specified axis
// around the specified point. Lookup is not implemented.
// Creating the array is virtually free, accessing a tuple require some
// computation.

#ifndef vtkPeriodicDataArray_h
#define vtkPeriodicDataArray_h

#include "vtkMappedDataArray.h"   // Parent
#include "vtkDataArrayTemplate.h" // Template

template <class Scalar>
class vtkPeriodicDataArray:
public vtkTypeTemplate <vtkPeriodicDataArray <Scalar>, vtkMappedDataArray <Scalar> >
{
public:
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Initialize the mapped array with the original input data array.
  void InitializeArray(vtkDataArrayTemplate<Scalar>* inputData);

  // Description:
  // Initialize array with zero values
  void Initialize();

  // Description:
  // Copy tuples values, selected by ptIds into provided array
  void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output);

  // Description:
  // Copy tuples from id p1 to id p2 included into provided array
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output);

  // Description:
  // No effect
  void Squeeze();

  // Description:
  // Not implemented
  vtkArrayIterator *NewIterator();

  // Description:
  // Not implemented
  vtkIdType LookupValue(vtkVariant value);

  // Description:
  // Not implemented
  void LookupValue(vtkVariant value, vtkIdList *ids);

  // Description:
  // Not implemented
  vtkVariant GetVariantValue(vtkIdType idx);

  // Description:
  // Not implemented
  void ClearLookup();

  // Description:
  // Return tuple at location i.
  // Pointer valid until next call to this object
  double* GetTuple(vtkIdType i);

  // Description:
  // Copy tuple at location i into user provided array
  void GetTuple(vtkIdType i, double *tuple);

  // Description:
  // Not implemented
  vtkIdType LookupTypedValue(Scalar value);

  // Description:
  // Not implemented
  void LookupTypedValue(Scalar value, vtkIdList *ids);

  // Description:
  // Get value at index idx.
  // Warning, it internally call GetTupleValue,
  // so it is an inneficcient way if reading all data
  Scalar GetValue(vtkIdType idx);

  // Description:
  // Get value at index idx as reference.
  // Warning, it internally call GetTupleValue,
  // so it is an inneficcient way if reading all data
  Scalar& GetValueReference(vtkIdType idx);

  // Description:
  // Copy tuple value at location idx into provided array
  void GetTupleValue(vtkIdType idx, Scalar *t);

  // Description:
  // Return the memory in kilobytes consumed by this data array.
  unsigned long GetActualMemorySize();

  // Description:
  // Read only container, not supported.
  int Allocate(vtkIdType sz, vtkIdType ext);

  // Description:
  // Read only container, not supported.
  int Resize(vtkIdType numTuples);

  // Description:
  // Read only container, not supported.
  void SetNumberOfTuples(vtkIdType number);

  // Description:
  // Read only container, not supported.
  void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source);

  // Description:
  // Read only container, not supported.
  void SetTuple(vtkIdType i, const float *source);

  // Description:
  // Read only container, not supported.
  void SetTuple(vtkIdType i, const double *source);

  // Description:
  // Read only container, not supported.
  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source);

  // Description:
  // Read only container, not supported.
  void InsertTuple(vtkIdType i, const float *source);

  // Description:
  // Read only container, not supported.
  void InsertTuple(vtkIdType i, const double *source);

  // Description:
  // Read only container, not supported.
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source);

  // Description:
  // Read only container, not supported.
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray* source);

  // Description:
  // Read only container, error.
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray *source);

  // Description:
  // Read only container, not supported.
  vtkIdType InsertNextTuple(const float *source);

  // Description:
  // Read only container, not supported.
  vtkIdType InsertNextTuple(const double *source);

  // Description:
  // Read only container, not supported.
  void DeepCopy(vtkAbstractArray *aa);

  // Description:
  // Read only container, not supported.
  void DeepCopy(vtkDataArray *da);

  // Description:
  // Read only container, not supported.
  void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
                        vtkAbstractArray* source, double* weights);

  // Description:
  // Read only container, not supported.
  void InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray *source1,
                        vtkIdType id2, vtkAbstractArray *source2, double t);

  // Description:
  // Read only container, not supported.
  void SetVariantValue(vtkIdType idx, vtkVariant value);

  // Description:
  // Read only container, not supported.
  void RemoveTuple(vtkIdType id);

  // Description:
  // Read only container, not supported.
  void RemoveFirstTuple();

  // Description:
  // Read only container, not supported.
  void RemoveLastTuple();

  // Description:
  // Read only container, not supported.
  void SetTupleValue(vtkIdType i, const Scalar *t);

  // Description:
  // Read only container, not supported.
  void InsertTupleValue(vtkIdType i, const Scalar *t);

  // Description:
  // Read only container, not supported.
  vtkIdType InsertNextTupleValue(const Scalar *t);

  // Description:
  // Read only container, not supported.
  void SetValue(vtkIdType idx, Scalar value);

  // Description:
  // Read only container, not supported.
  vtkIdType InsertNextValue(Scalar v);

  // Description:
  // Read only container, not supported.
  void InsertValue(vtkIdType idx, Scalar v);

  // Description:
  // Set/Get normalize flag. Default: false
  vtkSetMacro(Normalize, bool);
  vtkGetMacro(Normalize, bool);

protected:
  vtkPeriodicDataArray();
  ~vtkPeriodicDataArray();

  // Description:
  // Transform the provided tuple
  virtual void Transform(Scalar* tuple) = 0;

  // Description:
  // Get the transformed range by components
  virtual bool ComputeScalarRange(double* range);

  // Description:
  // Get the transformed range on all components
  virtual bool ComputeVectorRange(double range[2]);

  // Description:
  // Update the transformed periodic range
  virtual void ComputePeriodicRange();

  // Description:
  // Set the invalid range flag to false
  void InvalidateRange();

  bool Normalize; // If transformed vector must be normalized

private:
  vtkPeriodicDataArray(const vtkPeriodicDataArray &); // Not implemented.
  void operator=(const vtkPeriodicDataArray &); // Not implemented.

  Scalar* TempScalarArray; // Temporary array used by GetTupleValue methods
  double* TempDoubleArray; // Temporary array used by GetTuple vethods
  vtkIdType TempTupleIdx;  // Location of currently stored Temp Tuple to use as cache
  vtkDataArrayTemplate<Scalar>* Data; // Original data

  bool InvalidRange;
  double PeriodicRange[6]; // Transformed periodic range
};

#include "vtkPeriodicDataArray.txx"

#endif //vtkPeriodicDataArray_h
// VTK-HeaderTest-Exclude: vtkPeriodicDataArray.h
