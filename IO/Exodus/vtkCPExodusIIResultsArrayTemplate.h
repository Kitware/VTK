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

// .NAME vtkCPExodusIIResultsArrayTemplate - Map native Exodus II results arrays
// into the vtkDataArray interface.
//
// .SECTION Description
// Map native Exodus II results arrays into the vtkDataArray interface. Use
// the vtkCPExodusIIInSituReader to read an Exodus II file's data into this
// structure.

#ifndef __vtkCPExodusIIResultsArrayTemplate_h
#define __vtkCPExodusIIResultsArrayTemplate_h

#include "vtkMappedDataArray.h"

#include "vtkTypeTemplate.h" // For templated vtkObject API
#include "vtkObjectFactory.h" // for vtkStandardNewMacro

template <class Scalar>
class vtkCPExodusIIResultsArrayTemplate:
    public vtkTypeTemplate<vtkCPExodusIIResultsArrayTemplate<Scalar>,
                           vtkMappedDataArray<Scalar> >
{
public:
  vtkMappedDataArrayNewInstanceMacro(vtkCPExodusIIResultsArrayTemplate<Scalar>)
  static vtkCPExodusIIResultsArrayTemplate *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  void SetExodusScalarArrays(std::vector<Scalar*> arrays, vtkIdType numTuples);

  // Reimplemented virtuals -- see superclasses for descriptions:
  void Initialize();
  void GetTuples(vtkIdList *ptIds, vtkAbstractArray *output);
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output);
  void Squeeze();
  vtkArrayIterator *NewIterator();
  vtkIdType LookupValue(vtkVariant value);
  void LookupValue(vtkVariant value, vtkIdList *ids);
  vtkVariant GetVariantValue(vtkIdType idx);
  void ClearLookup();
  double* GetTuple(vtkIdType i);
  void GetTuple(vtkIdType i, double *tuple);
  vtkIdType LookupTypedValue(Scalar value);
  void LookupTypedValue(Scalar value, vtkIdList *ids);
  Scalar GetValue(vtkIdType idx);
  Scalar& GetValueReference(vtkIdType idx);
  void GetTupleValue(vtkIdType idx, Scalar *t);

  // Description:
  // This container is read only -- this method does nothing but print a
  // warning.
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
  void RemoveTuple(vtkIdType id);
  void RemoveFirstTuple();
  void RemoveLastTuple();
  void SetTupleValue(vtkIdType i, const Scalar *t);
  void InsertTupleValue(vtkIdType i, const Scalar *t);
  vtkIdType InsertNextTupleValue(const Scalar *t);
  void SetValue(vtkIdType idx, Scalar value);
  vtkIdType InsertNextValue(Scalar v);
  void InsertValue(vtkIdType idx, Scalar v);

protected:
  vtkCPExodusIIResultsArrayTemplate();
  ~vtkCPExodusIIResultsArrayTemplate();

  std::vector<Scalar *> Arrays;

private:
  vtkCPExodusIIResultsArrayTemplate(const vtkCPExodusIIResultsArrayTemplate &); // Not implemented.
  void operator=(const vtkCPExodusIIResultsArrayTemplate &); // Not implemented.

  vtkIdType Lookup(const Scalar &val, vtkIdType startIndex);
  double *TempDoubleArray;
};

#include "vtkCPExodusIIResultsArrayTemplate.txx"

#endif //__vtkCPExodusIIResultsArrayTemplate_h

// VTK-HeaderTest-Exclude: vtkCPExodusIIResultsArrayTemplate.h
