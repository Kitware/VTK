// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkCPExodusIIResultsArrayTemplate
 * @brief   Map native Exodus II results arrays
 * into the vtkDataArray interface.
 *
 *
 * Map native Exodus II results arrays into the vtkDataArray interface. Use
 * the vtkCPExodusIIInSituReader to read an Exodus II file's data into this
 * structure.
 */

#ifndef vtkCPExodusIIResultsArrayTemplate_h
#define vtkCPExodusIIResultsArrayTemplate_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_5_0
#include "vtkMappedDataArray.h"

#include "vtkObjectFactory.h" // for vtkStandardNewMacro

VTK_ABI_NAMESPACE_BEGIN
template <class Scalar>
class VTK_DEPRECATED_IN_9_5_0("Please use the SetArray functionality of `vtkAOSDataArrayTemplate` "
                              "for 1 component or `vtkSOADataArrayTemplate` for more "
                              "instead.") vtkCPExodusIIResultsArrayTemplate
  : public vtkMappedDataArray<Scalar>
{
public:
  vtkAbstractTemplateTypeMacro(
    vtkCPExodusIIResultsArrayTemplate<Scalar>, vtkMappedDataArray<Scalar>)
  vtkMappedDataArrayNewInstanceMacro(
    vtkCPExodusIIResultsArrayTemplate<Scalar>) static vtkCPExodusIIResultsArrayTemplate* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  typedef typename Superclass::ValueType ValueType;

  ///@{
  /**
   * Set the arrays to be used and the number of tuples in each array.
   * The save option can be set to true to indicate that this class
   * should not delete the actual allocated memory. By default it will
   * delete the array with the 'delete []' method.
   */
  void SetExodusScalarArrays(std::vector<Scalar*> arrays, vtkIdType numTuples);
  void SetExodusScalarArrays(std::vector<Scalar*> arrays, vtkIdType numTuples, bool save);
  ///@}

  // Reimplemented virtuals -- see superclasses for descriptions:
  void Initialize() override;
  void GetTuples(vtkIdList* ptIds, vtkAbstractArray* output) override;
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray* output) override;
  void Squeeze() override;
  VTK_NEWINSTANCE vtkArrayIterator* NewIterator() override;
  vtkIdType LookupValue(vtkVariant value) override;
  void LookupValue(vtkVariant value, vtkIdList* ids) override;
  vtkVariant GetVariantValue(vtkIdType idx) override;
  void ClearLookup() override;
  double* GetTuple(vtkIdType i) override;
  void GetTuple(vtkIdType i, double* tuple) override;
  vtkIdType LookupTypedValue(Scalar value) override;
  void LookupTypedValue(Scalar value, vtkIdList* ids) override;
  ValueType GetValue(vtkIdType idx) const override;
  ValueType& GetValueReference(vtkIdType idx) override;
  void GetTypedTuple(vtkIdType idx, Scalar* t) const override;

  ///@{
  /**
   * This container is read only -- this method does nothing but print a
   * warning.
   */
  vtkTypeBool Allocate(vtkIdType sz, vtkIdType ext) override;
  vtkTypeBool Resize(vtkIdType numTuples) override;
  void SetNumberOfTuples(vtkIdType number) override;
  void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) override;
  void SetTuple(vtkIdType i, const float* source) override;
  void SetTuple(vtkIdType i, const double* source) override;
  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) override;
  void InsertTuple(vtkIdType i, const float* source) override;
  void InsertTuple(vtkIdType i, const double* source) override;
  void InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source) override;
  void InsertTuplesStartingAt(
    vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* source) override;
  void InsertTuples(
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source) override;
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source) override;
  vtkIdType InsertNextTuple(const float* source) override;
  vtkIdType InsertNextTuple(const double* source) override;
  void DeepCopy(vtkAbstractArray* aa) override;
  void DeepCopy(vtkDataArray* da) override;
  void InterpolateTuple(
    vtkIdType i, vtkIdList* ptIndices, vtkAbstractArray* source, double* weights) override;
  void InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray* source1, vtkIdType id2,
    vtkAbstractArray* source2, double t) override;
  void SetVariantValue(vtkIdType idx, vtkVariant value) override;
  void InsertVariantValue(vtkIdType idx, vtkVariant value) override;
  void RemoveTuple(vtkIdType id) override;
  void RemoveFirstTuple() override;
  void RemoveLastTuple() override;
  void SetTypedTuple(vtkIdType i, const Scalar* t) override;
  void InsertTypedTuple(vtkIdType i, const Scalar* t) override;
  vtkIdType InsertNextTypedTuple(const Scalar* t) override;
  void SetValue(vtkIdType idx, Scalar value) override;
  vtkIdType InsertNextValue(Scalar v) override;
  void InsertValue(vtkIdType idx, Scalar v) override;
  ///@}

protected:
  vtkCPExodusIIResultsArrayTemplate();
  ~vtkCPExodusIIResultsArrayTemplate() override;

  std::vector<Scalar*> Arrays;

private:
  vtkCPExodusIIResultsArrayTemplate(const vtkCPExodusIIResultsArrayTemplate&) = delete;
  void operator=(const vtkCPExodusIIResultsArrayTemplate&) = delete;

  vtkIdType Lookup(const Scalar& val, vtkIdType startIndex);
  double* TempDoubleArray;
  ///@{
  /**
   * By default Save is false.
   */
  bool Save;
  ///@}
};

VTK_ABI_NAMESPACE_END
#include "vtkCPExodusIIResultsArrayTemplate.txx"

#endif // vtkCPExodusIIResultsArrayTemplate_h

// VTK-HeaderTest-Exclude: vtkCPExodusIIResultsArrayTemplate.h
