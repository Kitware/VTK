/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPExodusIINodalCoordinatesTemplate.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCPExodusIINodalCoordinatesTemplate.h"

#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkVariant.h"
#include "vtkVariantCast.h"

//------------------------------------------------------------------------------
// Can't use vtkStandardNewMacro with a template.
template <class Scalar> vtkCPExodusIINodalCoordinatesTemplate<Scalar> *
vtkCPExodusIINodalCoordinatesTemplate<Scalar>::New()
{
  VTK_STANDARD_NEW_BODY(vtkCPExodusIINodalCoordinatesTemplate<Scalar>)
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::PrintSelf(ostream &os, vtkIndent indent)
{
  this->vtkCPExodusIINodalCoordinatesTemplate<Scalar>::Superclass::PrintSelf(
        os, indent);
  os << indent << "XArray: " << this->XArray << std::endl;
  os << indent << "YArray: " << this->YArray << std::endl;
  os << indent << "ZArray: " << this->ZArray << std::endl;
  os << indent << "TempDoubleArray: " << this->TempDoubleArray << std::endl;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::Initialize()
{
  delete [] this->XArray;
  this->XArray = nullptr;
  delete [] this->YArray;
  this->YArray = nullptr;
  delete [] this->ZArray;
  this->ZArray = nullptr;
  delete [] this->TempDoubleArray;
  this->TempDoubleArray = nullptr;
  this->MaxId = -1;
  this->Size = 0;
  this->NumberOfComponents = 1;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::GetTuples(vtkIdList *ptIds, vtkAbstractArray *output)
{
  vtkDataArray *outArray = vtkDataArray::FastDownCast(output);
  if (!outArray)
  {
    vtkWarningMacro(<<"Input is not a vtkDataArray");
    return;
  }

  vtkIdType numTuples = ptIds->GetNumberOfIds();

  outArray->SetNumberOfComponents(this->NumberOfComponents);
  outArray->SetNumberOfTuples(numTuples);

  const vtkIdType numPoints = ptIds->GetNumberOfIds();
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    outArray->SetTuple(i, this->GetTuple(ptIds->GetId(i)));
  }
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *output)
{
  vtkDataArray *da = vtkDataArray::FastDownCast(output);
  if (!da)
  {
    vtkErrorMacro(<<"Input is not a vtkDataArray");
    return;
  }

  if (da->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkErrorMacro(<<"Incorrect number of components in input array.");
    return;
  }

  for (vtkIdType daTupleId = 0; p1 <= p2; ++p1)
  {
    da->SetTuple(daTupleId++, this->GetTuple(p1));
  }
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::Squeeze()
{
  // noop
}

//------------------------------------------------------------------------------
template <class Scalar> vtkArrayIterator*
vtkCPExodusIINodalCoordinatesTemplate<Scalar>::NewIterator()
{
  vtkErrorMacro(<<"Not implemented.");
  return nullptr;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::LookupValue(vtkVariant value)
{
  bool valid = true;
  Scalar val = vtkVariantCast<Scalar>(value, &valid);
  if (valid)
  {
    return this->Lookup(val, 0);
  }
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::LookupValue(vtkVariant value, vtkIdList *ids)
{
  bool valid = true;
  Scalar val = vtkVariantCast<Scalar>(value, &valid);
  ids->Reset();
  if (valid)
  {
    vtkIdType index = 0;
    while ((index = this->Lookup(val, index)) >= 0)
    {
      ids->InsertNextId(index++);
    }
  }
}

//------------------------------------------------------------------------------
template <class Scalar> vtkVariant vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::GetVariantValue(vtkIdType idx)
{
  return vtkVariant(this->GetValueReference(idx));
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::ClearLookup()
{
  // no-op, no fast lookup implemented.
}

//------------------------------------------------------------------------------
template <class Scalar> double* vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::GetTuple(vtkIdType i)
{
  this->GetTuple(i, this->TempDoubleArray);
  return this->TempDoubleArray;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::GetTuple(vtkIdType i, double *tuple)
{
  tuple[0] = static_cast<double>(this->XArray[i]);
  tuple[1] = static_cast<double>(this->YArray[i]);
  if (this->ZArray != nullptr)
  {
    tuple[2] = static_cast<double>(this->ZArray[i]);
  }
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::LookupTypedValue(Scalar value)
{
  return this->Lookup(value, 0);
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::LookupTypedValue(Scalar value, vtkIdList *ids)
{
  ids->Reset();
  vtkIdType index = 0;
  while ((index = this->Lookup(value, index)) >= 0)
  {
    ids->InsertNextId(index++);
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
typename vtkCPExodusIINodalCoordinatesTemplate<Scalar>::ValueType
vtkCPExodusIINodalCoordinatesTemplate<Scalar>::GetValue(vtkIdType idx) const
{
  return const_cast<vtkCPExodusIINodalCoordinatesTemplate<Scalar>*>(
        this)->GetValueReference(idx);
}

//------------------------------------------------------------------------------
template <class Scalar>
typename vtkCPExodusIINodalCoordinatesTemplate<Scalar>::ValueType&
vtkCPExodusIINodalCoordinatesTemplate<Scalar>::GetValueReference(vtkIdType idx)
{
  const vtkIdType tuple = idx / this->NumberOfComponents;
  const vtkIdType comp = idx % this->NumberOfComponents;
  switch (comp)
  {
    case 0:
      return this->XArray[tuple];
    case 1:
      return this->YArray[tuple];
    case 2:
      return this->ZArray[tuple];
    default:
      vtkErrorMacro(<< "Invalid number of components.");
      static Scalar dummy(0);
      return dummy;
  }
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::GetTypedTuple(vtkIdType tupleId, Scalar *tuple) const
{
  tuple[0] = this->XArray[tupleId];
  tuple[1] = this->YArray[tupleId];
  if (this->ZArray != nullptr)
  {
    tuple[2] = this->ZArray[tupleId];
  }
}

//------------------------------------------------------------------------------
template <class Scalar> vtkTypeBool vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::Allocate(vtkIdType, vtkIdType)
{
  vtkErrorMacro("Read only container.")
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkTypeBool vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::Resize(vtkIdType)
{
  vtkErrorMacro("Read only container.")
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::SetNumberOfTuples(vtkIdType)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::SetTuple(vtkIdType, vtkIdType, vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::SetTuple(vtkIdType, const float *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::SetTuple(vtkIdType, const double *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertTuple(vtkIdType, vtkIdType, vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertTuple(vtkIdType, const float *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertTuple(vtkIdType, const double *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertTuples(vtkIdList *, vtkIdList *, vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertTuples(vtkIdType, vtkIdType, vtkIdType, vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertNextTuple(vtkIdType, vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertNextTuple(const float *)
{

  vtkErrorMacro("Read only container.")
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertNextTuple(const double *)
{
  vtkErrorMacro("Read only container.")
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::DeepCopy(vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::DeepCopy(vtkDataArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InterpolateTuple(vtkIdType, vtkIdList *, vtkAbstractArray *, double *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InterpolateTuple(vtkIdType, vtkIdType, vtkAbstractArray*, vtkIdType,
                   vtkAbstractArray*, double)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::SetVariantValue(vtkIdType, vtkVariant)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertVariantValue(vtkIdType, vtkVariant)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::RemoveTuple(vtkIdType)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::RemoveFirstTuple()
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::RemoveLastTuple()
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::SetTypedTuple(vtkIdType, const Scalar*)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertTypedTuple(vtkIdType, const Scalar*)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertNextTypedTuple(const Scalar *)
{
  vtkErrorMacro("Read only container.")
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::SetValue(vtkIdType, Scalar)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertNextValue(Scalar)
{
  vtkErrorMacro("Read only container.")
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::InsertValue(vtkIdType, Scalar)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::vtkCPExodusIINodalCoordinatesTemplate()
  : XArray(nullptr),
    YArray(nullptr),
    ZArray(nullptr),
    TempDoubleArray(nullptr)
{
}

//------------------------------------------------------------------------------
template <class Scalar> vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::~vtkCPExodusIINodalCoordinatesTemplate()
{
  delete [] this->XArray;
  delete [] this->YArray;
  delete [] this->ZArray;
  delete [] this->TempDoubleArray;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::SetExodusScalarArrays(Scalar *x, Scalar *y, Scalar *z, vtkIdType numPoints)
{
  Initialize();
  this->XArray = x;
  this->YArray = y;
  this->ZArray = z;
  this->NumberOfComponents = (z != nullptr) ? 3 : 2;
  this->Size = this->NumberOfComponents * numPoints;
  this->MaxId = this->Size - 1;
  this->TempDoubleArray = new double [this->NumberOfComponents];
  this->Modified();
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIINodalCoordinatesTemplate<Scalar>
::Lookup(const Scalar &val, vtkIdType index)
{
  while (index <= this->MaxId)
  {
    if (this->GetValueReference(index++) == val)
    {
      return index;
    }
  }
  return -1;
}
