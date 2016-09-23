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
  virtual void PrintSelf(ostream &os, vtkIndent indent);

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
  void Initialize();
  void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output);
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output);
  void Squeeze();
  VTK_NEWINSTANCE vtkArrayIterator *NewIterator();
  vtkIdType LookupValue(vtkVariant value);
  void LookupValue(vtkVariant value, vtkIdList *ids);
  vtkVariant GetVariantValue(vtkIdType idx);
  void ClearLookup();
  double* GetTuple(vtkIdType i);
  void GetTuple(vtkIdType i, double *tuple);
  vtkIdType LookupTypedValue(Scalar value);
  void LookupTypedValue(Scalar value, vtkIdList *ids);
  ValueType GetValue(vtkIdType idx) const;
  ValueType& GetValueReference(vtkIdType idx);
  void GetTypedTuple(vtkIdType idx, Scalar *t) const;

  //@{
  /**
   * This container is read only -- this method does nothing but print a
   * warning.
   */
  int Allocate(vtkIdType sz, vtkIdType ext);
  int Resize(vtkIdType numTuples);
  void SetNumberOfTuples(vtkIdType number);
  void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source);
  void SetTuple(vtkIdType i, const float *source);
  void SetTuple(vtkIdType i, const double *source);
  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source);
  void InsertTuple(vtkIdType i, const float *source);
  void InsertTuple(vtkIdType i, const double *source);
  void InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds,
                    vtkAbstractArray *source);
  void InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
                    vtkAbstractArray* source);
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray *source);
  vtkIdType InsertNextTuple(const float *source);
  vtkIdType InsertNextTuple(const double *source);
  void DeepCopy(vtkAbstractArray *aa);
  void DeepCopy(vtkDataArray *da);
  void InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
                        vtkAbstractArray* source,  double* weights);
  void InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray *source1,
                        vtkIdType id2, vtkAbstractArray *source2, double t);
  void SetVariantValue(vtkIdType idx, vtkVariant value);
  void InsertVariantValue(vtkIdType idx, vtkVariant value);
  void RemoveTuple(vtkIdType id);
  void RemoveFirstTuple();
  void RemoveLastTuple();
  void SetTypedTuple(vtkIdType i, const Scalar *t);
  void InsertTypedTuple(vtkIdType i, const Scalar *t);
  vtkIdType InsertNextTypedTuple(const Scalar *t);
  void SetValue(vtkIdType idx, Scalar value);
  vtkIdType InsertNextValue(Scalar v);
  void InsertValue(vtkIdType idx, Scalar v);
  //@}

protected:
  vtkCPExodusIIResultsArrayTemplate();
  ~vtkCPExodusIIResultsArrayTemplate();

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
