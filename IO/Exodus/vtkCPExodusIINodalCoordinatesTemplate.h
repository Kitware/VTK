/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPExodusIINodalCoordinatesTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkCPExodusIINodalCoordinatesTemplate
 * @brief   Map native Exodus II coordinate
 * arrays into the vtkDataArray interface.
 *
 *
 * Map native Exodus II coordinate arrays into the vtkDataArray interface. Use
 * the vtkCPExodusIIInSituReader to read an Exodus II file's data into this
 * structure.
*/

#ifndef vtkCPExodusIINodalCoordinatesTemplate_h
#define vtkCPExodusIINodalCoordinatesTemplate_h

#include "vtkMappedDataArray.h"
#include "vtkIOExodusModule.h" // For export macro

#include "vtkObjectFactory.h" // for vtkStandardNewMacro

template <class Scalar>
class vtkCPExodusIINodalCoordinatesTemplate: public vtkMappedDataArray<Scalar>
{
public:
  vtkAbstractTemplateTypeMacro(vtkCPExodusIINodalCoordinatesTemplate<Scalar>,
                               vtkMappedDataArray<Scalar>)
  vtkMappedDataArrayNewInstanceMacro(
      vtkCPExodusIINodalCoordinatesTemplate<Scalar>)
  static vtkCPExodusIINodalCoordinatesTemplate *New();
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  typedef typename Superclass::ValueType ValueType;

  /**
   * Set the raw scalar arrays for the coordinate set. This class takes
   * ownership of the arrays and deletes them with delete[].
   */
  void SetExodusScalarArrays(Scalar *x, Scalar *y, Scalar *z,
                             vtkIdType numPoints);

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
  vtkCPExodusIINodalCoordinatesTemplate();
  ~vtkCPExodusIINodalCoordinatesTemplate() VTK_OVERRIDE;

  Scalar *XArray;
  Scalar *YArray;
  Scalar *ZArray;

private:
  vtkCPExodusIINodalCoordinatesTemplate(
      const vtkCPExodusIINodalCoordinatesTemplate &) VTK_DELETE_FUNCTION;
  void operator=(
      const vtkCPExodusIINodalCoordinatesTemplate &) VTK_DELETE_FUNCTION;

  vtkIdType Lookup(const Scalar &val, vtkIdType startIndex);
  double *TempDoubleArray;
};

#include "vtkCPExodusIINodalCoordinatesTemplate.txx"

#endif //vtkCPExodusIINodalCoordinatesTemplate_h

// VTK-HeaderTest-Exclude: vtkCPExodusIINodalCoordinatesTemplate.h
