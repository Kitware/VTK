/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPExodusIIResultsArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

#include "vtkMappedDataArray.h"

#include "vtkObjectFactory.h" // for vtkStandardNewMacro

template <class Scalar>
class vtkCPExodusIIResultsArrayTemplate: public vtkMappedDataArray<Scalar>
{
public:
  vtkAbstractTemplateTypeMacro(vtkCPExodusIIResultsArrayTemplate<Scalar>,
                               vtkMappedDataArray<Scalar>)
  vtkMappedDataArrayNewInstanceMacro(vtkCPExodusIIResultsArrayTemplate<Scalar>)
  static vtkCPExodusIIResultsArrayTemplate *New();
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  typedef typename Superclass::ValueType ValueType;

  //@{
  /**
   * Set the arrays to be used and the number of tuples in each array.
   * The save option can be set to true to indicate that this class
   * should not delete the actual allocated memory. By default it will
   * delete the array with the 'delete []' method.
   */
  void SetExodusScalarArrays(std::vector<Scalar*> arrays, vtkIdType numTuples);
  void SetExodusScalarArrays(std::vector<Scalar*> arrays, vtkIdType numTuples, bool save);
  //@}

  // Reimplemented virtuals -- see superclasses for descriptions:
  void Initialize() VTK_OVERRIDE;
  void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output) VTK_OVERRIDE;
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output) VTK_OVERRIDE;
  void Squeeze() VTK_OVERRIDE;
  VTK_NEWINSTANCE vtkArrayIterator *NewIterator() VTK_OVERRIDE;
  vtkIdType LookupValue(vtkVariant value) VTK_OVERRIDE;
  void LookupValue(vtkVariant value, vtkIdList *ids) VTK_OVERRIDE;
  vtkVariant GetVariantValue(vtkIdType idx) VTK_OVERRIDE;
  void ClearLookup() VTK_OVERRIDE;
  double* GetTuple(vtkIdType i) VTK_OVERRIDE;
  void GetTuple(vtkIdType i, double *tuple) VTK_OVERRIDE;
  vtkIdType LookupTypedValue(Scalar value) VTK_OVERRIDE;
  void LookupTypedValue(Scalar value, vtkIdList *ids) VTK_OVERRIDE;
  ValueType GetValue(vtkIdType idx) const VTK_OVERRIDE;
  ValueType& GetValueReference(vtkIdType idx) VTK_OVERRIDE;
  void GetTypedTuple(vtkIdType idx, Scalar *t) const VTK_OVERRIDE;

  //@{
  /**
   * This container is read only -- this method does nothing but print a
   * warning.
   */
  int Allocate(vtkIdType sz, vtkIdType ext) VTK_OVERRIDE;
  int Resize(vtkIdType numTuples) VTK_OVERRIDE;
  void SetNumberOfTuples(vtkIdType number) VTK_OVERRIDE;
  void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source) VTK_OVERRIDE;
  void SetTuple(vtkIdType i, const float *source) VTK_OVERRIDE;
  void SetTuple(vtkIdType i, const double *source) VTK_OVERRIDE;
  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source) VTK_OVERRIDE;
  void InsertTuple(vtkIdType i, const float *source) VTK_OVERRIDE;
  void InsertTuple(vtkIdType i, const double *source) VTK_OVERRIDE;
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source) VTK_OVERRIDE;
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray* source) VTK_OVERRIDE;
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray *source) VTK_OVERRIDE;
  vtkIdType InsertNextTuple(const float *source) VTK_OVERRIDE;
  vtkIdType InsertNextTuple(const double *source) VTK_OVERRIDE;
  void DeepCopy(vtkAbstractArray *aa) VTK_OVERRIDE;
  void DeepCopy(vtkDataArray *da) VTK_OVERRIDE;
  void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
                        vtkAbstractArray* source,  double* weights) VTK_OVERRIDE;
  void InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray *source1,
                        vtkIdType id2, vtkAbstractArray *source2, double t) VTK_OVERRIDE;
  void SetVariantValue(vtkIdType idx, vtkVariant value) VTK_OVERRIDE;
  void InsertVariantValue(vtkIdType idx, vtkVariant value) VTK_OVERRIDE;
  void RemoveTuple(vtkIdType id) VTK_OVERRIDE;
  void RemoveFirstTuple() VTK_OVERRIDE;
  void RemoveLastTuple() VTK_OVERRIDE;
  void SetTypedTuple(vtkIdType i, const Scalar *t) VTK_OVERRIDE;
  void InsertTypedTuple(vtkIdType i, const Scalar *t) VTK_OVERRIDE;
  vtkIdType InsertNextTypedTuple(const Scalar *t) VTK_OVERRIDE;
  void SetValue(vtkIdType idx, Scalar value) VTK_OVERRIDE;
  vtkIdType InsertNextValue(Scalar v) VTK_OVERRIDE;
  void InsertValue(vtkIdType idx, Scalar v) VTK_OVERRIDE;
  //@}

protected:
  vtkCPExodusIIResultsArrayTemplate();
  ~vtkCPExodusIIResultsArrayTemplate() VTK_OVERRIDE;

  std::vector<Scalar *> Arrays;

private:
  vtkCPExodusIIResultsArrayTemplate(const vtkCPExodusIIResultsArrayTemplate &) VTK_DELETE_FUNCTION;
  void operator=(const vtkCPExodusIIResultsArrayTemplate &) VTK_DELETE_FUNCTION;

  vtkIdType Lookup(const Scalar &val, vtkIdType startIndex);
  double *TempDoubleArray;
  //@{
  /**
   * By default Save is false.
   */
  bool Save;
};
  //@}

#include "vtkCPExodusIIResultsArrayTemplate.txx"

#endif //vtkCPExodusIIResultsArrayTemplate_h

// VTK-HeaderTest-Exclude: vtkCPExodusIIResultsArrayTemplate.h
