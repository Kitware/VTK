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

/**
 * @class   vtkPeriodicDataArray
 * @brief   Map native an Array into an angulat
 * periodic array
 *
 *
 * Map an array into a periodic array. Data from the original array are
 * rotated (on the fly) by the specified angle along the specified axis
 * around the specified point. Lookup is not implemented.
 * Creating the array is virtually free, accessing a tuple require some
 * computation.
*/

#ifndef vtkPeriodicDataArray_h
#define vtkPeriodicDataArray_h

#include "vtkGenericDataArray.h"   // Parent
#include "vtkAOSDataArrayTemplate.h" // Template

template <class Scalar>
class vtkPeriodicDataArray:
    public vtkGenericDataArray<vtkPeriodicDataArray<Scalar>, Scalar>
{
  typedef vtkGenericDataArray<vtkPeriodicDataArray<Scalar>, Scalar> GenericBase;
public:
  vtkTemplateTypeMacro(vtkPeriodicDataArray<Scalar>, GenericBase)
  typedef typename Superclass::ValueType ValueType;

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the mapped array with the original input data array.
   */
  void InitializeArray(vtkAOSDataArrayTemplate<Scalar>* inputData);

  /**
   * Initialize array with zero values
   */
  void Initialize() override;

  /**
   * Copy tuples values, selected by ptIds into provided array
   */
  void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output) override;

  /**
   * Copy tuples from id p1 to id p2 included into provided array
   */
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output) override;

  /**
   * No effect
   */
  void Squeeze() override;

  /**
   * Not implemented
   */
  VTK_NEWINSTANCE vtkArrayIterator *NewIterator() override;

  /**
   * Not implemented
   */
  vtkIdType LookupValue(vtkVariant value) override;

  /**
   * Not implemented
   */
  void LookupValue(vtkVariant value, vtkIdList *ids) override;

  /**
   * Not implemented
   */
  vtkVariant GetVariantValue(vtkIdType idx) override;

  /**
   * Not implemented
   */
  void ClearLookup() override;

  /**
   * Return tuple at location i.
   * Pointer valid until next call to this object
   */
  double* GetTuple(vtkIdType i) override;

  /**
   * Copy tuple at location i into user provided array
   */
  void GetTuple(vtkIdType i, double *tuple) override;

  /**
   * Not implemented
   */
  vtkIdType LookupTypedValue(Scalar value) override;

  /**
   * Not implemented
   */
  void LookupTypedValue(Scalar value, vtkIdList *ids) override;

  /**
   * Get value at index idx.
   * Warning, it internally call GetTypedTuple,
   * so it is an inneficcient way if reading all data
   */
  ValueType GetValue(vtkIdType idx) const ;

  /**
   * Get value at index idx as reference.
   * Warning, it internally call GetTypedTuple,
   * so it is an inneficcient way if reading all data
   */
  ValueType& GetValueReference(vtkIdType idx);

  /**
   * Copy tuple value at location idx into provided array
   */
  void GetTypedTuple(vtkIdType idx, Scalar *t) const;

  /**
   * Return the requested component of the specified tuple.
   * Warning, this internally calls GetTypedTuple, so it is an inefficient way
   * of reading all data.
   */
  ValueType GetTypedComponent(vtkIdType tupleIdx, int compIdx) const;

  /**
   * Return the memory in kilobytes consumed by this data array.
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Read only container, not supported.
   */
  vtkTypeBool Allocate(vtkIdType sz, vtkIdType ext) override;

  /**
   * Read only container, not supported.
   */
  vtkTypeBool Resize(vtkIdType numTuples) override;

  /**
   * Read only container, not supported.
   */
  void SetNumberOfTuples(vtkIdType number) override;

  /**
   * Read only container, not supported.
   */
  void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source) override;

  /**
   * Read only container, not supported.
   */
  void SetTuple(vtkIdType i, const float *source) override;

  /**
   * Read only container, not supported.
   */
  void SetTuple(vtkIdType i, const double *source) override;

  /**
   * Read only container, not supported.
   */
  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source) override;

  /**
   * Read only container, not supported.
   */
  void InsertTuple(vtkIdType i, const float *source) override;

  /**
   * Read only container, not supported.
   */
  void InsertTuple(vtkIdType i, const double *source) override;

  /**
   * Read only container, not supported.
   */
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source) override;

  /**
   * Read only container, not supported.
   */
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray* source) override;

  /**
   * Read only container, error.
   */
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray *source) override;

  /**
   * Read only container, not supported.
   */
  vtkIdType InsertNextTuple(const float *source) override;

  /**
   * Read only container, not supported.
   */
  vtkIdType InsertNextTuple(const double *source) override;

  /**
   * Read only container, not supported.
   */
  void DeepCopy(vtkAbstractArray *aa) override;

  /**
   * Read only container, not supported.
   */
  void DeepCopy(vtkDataArray *da) override;

  /**
   * Read only container, not supported.
   */
  void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
                        vtkAbstractArray* source, double* weights) override;

  /**
   * Read only container, not supported.
   */
  void InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray *source1,
                        vtkIdType id2, vtkAbstractArray *source2, double t) override;

  /**
   * Read only container, not supported.
   */
  void SetVariantValue(vtkIdType idx, vtkVariant value) override;

  /**
   * Read only container, not supported.
   */
  void InsertVariantValue(vtkIdType idx, vtkVariant value) override;

  /**
   * Read only container, not supported.
   */
  void RemoveTuple(vtkIdType id) override;

  /**
   * Read only container, not supported.
   */
  void RemoveFirstTuple() override;

  /**
   * Read only container, not supported.
   */
  void RemoveLastTuple() override;

  /**
   * Read only container, not supported.
   */
  void SetTypedTuple(vtkIdType i, const Scalar *t);

  /**
   * Read only container, not supported.
   */
  void SetTypedComponent(vtkIdType t, int c, Scalar v);

  /**
   * Read only container, not supported.
   */
  void InsertTypedTuple(vtkIdType i, const Scalar *t);

  /**
   * Read only container, not supported.
   */
  vtkIdType InsertNextTypedTuple(const Scalar *t);

  /**
   * Read only container, not supported.
   */
  void SetValue(vtkIdType idx, Scalar value);

  /**
   * Read only container, not supported.
   */
  vtkIdType InsertNextValue(Scalar v);

  /**
   * Read only container, not supported.
   */
  void InsertValue(vtkIdType idx, Scalar v);

  //@{
  /**
   * Set/Get normalize flag. Default: false
   */
  vtkSetMacro(Normalize, bool);
  vtkGetMacro(Normalize, bool);
  //@}

protected:
  vtkPeriodicDataArray();
  ~vtkPeriodicDataArray() override;

  //@{
  /**
   * Read only container, not supported.
   */
  bool AllocateTuples(vtkIdType numTuples);
  bool ReallocateTuples(vtkIdType numTuples);
  //@}

  /**
   * Transform the provided tuple
   */
  virtual void Transform(Scalar* tuple) const = 0;

  /**
   * Get the transformed range by components
   */
  bool ComputeScalarRange(double* range) override;

  /**
   * Get the transformed range on all components
   */
  bool ComputeVectorRange(double range[2]) override;

  /**
   * Update the transformed periodic range
   */
  virtual void ComputePeriodicRange();

  /**
   * Set the invalid range flag to false
   */
  void InvalidateRange();

  bool Normalize; // If transformed vector must be normalized

private:
  vtkPeriodicDataArray(const vtkPeriodicDataArray &) = delete;
  void operator=(const vtkPeriodicDataArray &) = delete;

  friend class vtkGenericDataArray<vtkPeriodicDataArray<Scalar>, Scalar>;

  Scalar* TempScalarArray; // Temporary array used by GetTypedTuple methods
  double* TempDoubleArray; // Temporary array used by GetTuple vethods
  vtkIdType TempTupleIdx;  // Location of currently stored Temp Tuple to use as cache
  vtkAOSDataArrayTemplate<Scalar>* Data; // Original data

  bool InvalidRange;
  double PeriodicRange[6]; // Transformed periodic range
};

#include "vtkPeriodicDataArray.txx"

#endif //vtkPeriodicDataArray_h
// VTK-HeaderTest-Exclude: vtkPeriodicDataArray.h
