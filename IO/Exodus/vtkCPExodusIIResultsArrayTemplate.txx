/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPExodusIIResultsArrayTemplate.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCPExodusIIResultsArrayTemplate.h"

#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkVariant.h"
#include "vtkVariantCast.h"

//------------------------------------------------------------------------------
// Can't use vtkStandardNewMacro on a templated class.
template <class Scalar> vtkCPExodusIIResultsArrayTemplate<Scalar> *
vtkCPExodusIIResultsArrayTemplate<Scalar>::New()
{
  VTK_STANDARD_NEW_BODY(vtkCPExodusIIResultsArrayTemplate<Scalar>)
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::PrintSelf(ostream &os, vtkIndent indent)
{
  this->vtkCPExodusIIResultsArrayTemplate<Scalar>::Superclass::PrintSelf(
        os, indent);

  os << indent << "Number of arrays: " << this->Arrays.size() << "\n";
  vtkIndent deeper = indent.GetNextIndent();
  for (size_t i = 0; i < this->Arrays.size(); ++i)
    {
    os << deeper << "Array " << i << ": " << this->Arrays.at(i) << "\n";
    }

  os << indent << "TempDoubleArray: " << this->TempDoubleArray << "\n";
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::SetExodusScalarArrays(std::vector<Scalar *> arrays, vtkIdType numTuples)
{
  this->Initialize();
  this->NumberOfComponents = static_cast<int>(arrays.size());
  this->Arrays = arrays;
  this->Size = this->NumberOfComponents * numTuples;
  this->MaxId = this->Size - 1;
  this->TempDoubleArray = new double [this->NumberOfComponents];
  this->Modified();
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::Initialize()
{
  for (size_t i = 0; i < this->Arrays.size(); ++i)
    {
    delete this->Arrays[i];
    }
  this->Arrays.clear();
  this->Arrays.push_back(NULL);

  delete [] this->TempDoubleArray;
  this->TempDoubleArray = NULL;

  this->MaxId = -1;
  this->Size = 0;
  this->NumberOfComponents = 1;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::GetTuples(vtkIdList *ptIds, vtkAbstractArray *output)
{
  vtkDataArray *da = vtkDataArray::FastDownCast(output);
  if (!da)
    {
    vtkWarningMacro(<<"Input is not a vtkDataArray");
    return;
    }

  if (da->GetNumberOfComponents() != this->GetNumberOfComponents())
    {
    vtkWarningMacro(<<"Incorrect number of components in input array.");
    return;
    }

  const vtkIdType numPoints = ptIds->GetNumberOfIds();
  for (vtkIdType i = 0; i < numPoints; ++i)
    {
    da->SetTuple(i, this->GetTuple(ptIds->GetId(i)));
    }
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
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
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::Squeeze()
{
  // noop
}

//------------------------------------------------------------------------------
template <class Scalar> vtkArrayIterator*
vtkCPExodusIIResultsArrayTemplate<Scalar>::NewIterator()
{
  vtkErrorMacro(<<"Not implemented.");
  return NULL;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIIResultsArrayTemplate<Scalar>
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
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
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
      ids->InsertNextId(index);
      ++index;
      }
    }
}

//------------------------------------------------------------------------------
template <class Scalar> vtkVariant vtkCPExodusIIResultsArrayTemplate<Scalar>
::GetVariantValue(vtkIdType idx)
{
  return vtkVariant(this->GetValueReference(idx));
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::ClearLookup()
{
  // no-op, no fast lookup implemented.
}

//------------------------------------------------------------------------------
template <class Scalar> double* vtkCPExodusIIResultsArrayTemplate<Scalar>
::GetTuple(vtkIdType i)
{
  this->GetTuple(i, this->TempDoubleArray);
  return this->TempDoubleArray;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::GetTuple(vtkIdType i, double *tuple)
{
  for (size_t comp = 0; comp < this->Arrays.size(); ++comp)
    {
    tuple[comp] = static_cast<double>(this->Arrays[comp][i]);
    }
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIIResultsArrayTemplate<Scalar>
::LookupTypedValue(Scalar value)
{
  return this->Lookup(value, 0);
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::LookupTypedValue(Scalar value, vtkIdList *ids)
{
  ids->Reset();
  vtkIdType index = 0;
  while ((index = this->Lookup(value, index)) >= 0)
    {
    ids->InsertNextId(index);
    ++index;
    }
}

//------------------------------------------------------------------------------
template <class Scalar> Scalar vtkCPExodusIIResultsArrayTemplate<Scalar>
::GetValue(vtkIdType idx)
{
  return this->GetValueReference(idx);
}

//------------------------------------------------------------------------------
template <class Scalar> Scalar& vtkCPExodusIIResultsArrayTemplate<Scalar>
::GetValueReference(vtkIdType idx)
{
  const vtkIdType tuple = idx / this->NumberOfComponents;
  const vtkIdType comp = idx % this->NumberOfComponents;
  return this->Arrays[comp][tuple];
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::GetTupleValue(vtkIdType tupleId, Scalar *tuple)
{
  for (size_t comp = 0; comp < this->Arrays.size(); ++comp)
    {
    tuple[comp] = this->Arrays[comp][tupleId];
    }
}

//------------------------------------------------------------------------------
template <class Scalar> int vtkCPExodusIIResultsArrayTemplate<Scalar>
::Allocate(vtkIdType, vtkIdType)
{
  vtkErrorMacro("Read only container.")
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar> int vtkCPExodusIIResultsArrayTemplate<Scalar>
::Resize(vtkIdType)
{
  vtkErrorMacro("Read only container.")
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::SetNumberOfTuples(vtkIdType)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::SetTuple(vtkIdType, vtkIdType, vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::SetTuple(vtkIdType, const float *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::SetTuple(vtkIdType, const double *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertTuple(vtkIdType, vtkIdType, vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertTuple(vtkIdType, const float *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertTuple(vtkIdType, const double *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertTuples(vtkIdList *, vtkIdList *, vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertNextTuple(vtkIdType, vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertNextTuple(const float *)
{

  vtkErrorMacro("Read only container.")
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertNextTuple(const double *)
{
  vtkErrorMacro("Read only container.")
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::DeepCopy(vtkAbstractArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::DeepCopy(vtkDataArray *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::InterpolateTuple(vtkIdType, vtkIdList *, vtkAbstractArray *, double *)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::InterpolateTuple(vtkIdType, vtkIdType, vtkAbstractArray*, vtkIdType,
                   vtkAbstractArray*, double)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::SetVariantValue(vtkIdType, vtkVariant)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::RemoveTuple(vtkIdType)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::RemoveFirstTuple()
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::RemoveLastTuple()
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::SetTupleValue(vtkIdType, const Scalar*)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertTupleValue(vtkIdType, const Scalar*)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertNextTupleValue(const Scalar *)
{
  vtkErrorMacro("Read only container.")
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::SetValue(vtkIdType, Scalar)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertNextValue(Scalar)
{
  vtkErrorMacro("Read only container.")
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar> void vtkCPExodusIIResultsArrayTemplate<Scalar>
::InsertValue(vtkIdType, Scalar)
{
  vtkErrorMacro("Read only container.")
  return;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkCPExodusIIResultsArrayTemplate<Scalar>
::vtkCPExodusIIResultsArrayTemplate()
  : TempDoubleArray(NULL)
{
}

//------------------------------------------------------------------------------
template <class Scalar> vtkCPExodusIIResultsArrayTemplate<Scalar>
::~vtkCPExodusIIResultsArrayTemplate()
{
  typedef typename std::vector<Scalar*>::const_iterator ArrayIterator;
  for (ArrayIterator it = this->Arrays.begin(), itEnd = this->Arrays.end();
       it != itEnd; ++it)
    {
    delete [] *it;
    }
  delete [] this->TempDoubleArray;
}

//------------------------------------------------------------------------------
template <class Scalar> vtkIdType vtkCPExodusIIResultsArrayTemplate<Scalar>
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
