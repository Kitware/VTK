// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkArrayIteratorTemplate.h"
#include "vtkIdList.h"
#include "vtkVariant.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkPeriodicDataArray<Scalar>::Superclass::PrintSelf(os, indent);

  os << indent << "TempScalarArray: " << this->TempScalarArray << "\n";
  os << indent << "TempDoubleArray: " << this->TempDoubleArray << "\n";
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::Initialize()
{
  delete[] this->TempScalarArray;
  this->TempScalarArray = nullptr;
  delete[] this->TempDoubleArray;
  this->TempDoubleArray = nullptr;
  this->TempTupleIdx = -1;

  if (this->Data)
  {
    this->Data->Delete();
    this->Data = nullptr;
  }

  this->MaxId = -1;
  this->Size = 0;
  this->Normalize = false;
  this->Modified();
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InitializeArray(vtkAOSDataArrayTemplate<Scalar>* data)
{
  this->Initialize();
  if (!data)
  {
    vtkErrorMacro(<< "No original data provided.");
    return;
  }

  this->NumberOfComponents = data->GetNumberOfComponents();
  this->Size = data->GetSize();
  this->MaxId = data->GetMaxId();
  this->Data = data;
  this->Data->Register(nullptr);
  this->TempScalarArray = new Scalar[this->NumberOfComponents];
  this->TempDoubleArray = new double[this->NumberOfComponents];
  this->SetName(data->GetName());
  this->InvalidRange = true;
  this->Modified();
}

//------------------------------------------------------------------------------
template <class Scalar>
bool vtkPeriodicDataArray<Scalar>::ComputeScalarRange(double* range)
{
  if (this->NumberOfComponents == 3)
  {
    if (this->InvalidRange)
    {
      this->ComputePeriodicRange();
    }
    for (int i = 0; i < 3; i++)
    {
      range[i * 2] = this->PeriodicRange[i * 2 + 0];
      range[i * 2 + 1] = this->PeriodicRange[i * 2 + 1];
    }
  }
  else
  {
    // Not implemented for tensor
    for (int i = 0; i < this->NumberOfComponents; i++)
    {
      range[i * 2] = 0;
      range[i * 2 + 1] = 1;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
template <class Scalar>
bool vtkPeriodicDataArray<Scalar>::ComputeVectorRange(double range[2])
{
  if (this->NumberOfComponents == 3 && this->Data)
  {
    this->Data->GetRange(range, -1);
  }
  else
  {
    // Not implemented for tensor
    range[0] = 0;
    range[1] = 1;
  }
  return true;
}

//------------------------------------------------------------------------------
template <class Scalar>
bool vtkPeriodicDataArray<Scalar>::ComputeFiniteScalarRange(double* range)
{
  if (this->NumberOfComponents == 3)
  {
    if (this->InvalidFiniteRange)
    {
      this->ComputePeriodicRange(true);
    }
    for (int i = 0; i < 3; i++)
    {
      range[i * 2] = this->PeriodicFiniteRange[i * 2 + 0];
      range[i * 2 + 1] = this->PeriodicFiniteRange[i * 2 + 1];
    }
  }
  else
  {
    // Not implemented for tensor
    for (int i = 0; i < this->NumberOfComponents; i++)
    {
      range[i * 2] = 0;
      range[i * 2 + 1] = 1;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
template <class Scalar>
bool vtkPeriodicDataArray<Scalar>::ComputeFiniteVectorRange(double range[2])
{
  if (this->NumberOfComponents == 3 && this->Data)
  {
    this->Data->GetFiniteRange(range, -1);
  }
  else
  {
    // Not implemented for tensor
    range[0] = 0;
    range[1] = 1;
  }
  return true;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::ComputePeriodicRange(bool finite)
{
  double* range = finite ? this->PeriodicFiniteRange : this->PeriodicRange;

  if (this->Data)
  {
    if (finite)
    {
      this->Data->GetFiniteRange(range, 0);
      this->Data->GetFiniteRange(range + 2, 1);
      this->Data->GetFiniteRange(range + 4, 2);
    }
    else
    {
      this->Data->GetRange(range, 0);
      this->Data->GetRange(range + 2, 1);
      this->Data->GetRange(range + 4, 2);
    }

    Scalar boxPoints[8][3];
    boxPoints[0][0] = range[0];
    boxPoints[0][1] = range[2];
    boxPoints[0][2] = range[4];

    boxPoints[1][0] = range[0];
    boxPoints[1][1] = range[3];
    boxPoints[1][2] = range[4];

    boxPoints[2][0] = range[1];
    boxPoints[2][1] = range[3];
    boxPoints[2][2] = range[4];

    boxPoints[3][0] = range[1];
    boxPoints[3][1] = range[2];
    boxPoints[3][2] = range[4];

    boxPoints[4][0] = range[0];
    boxPoints[4][1] = range[2];
    boxPoints[4][2] = range[5];

    boxPoints[5][0] = range[0];
    boxPoints[5][1] = range[3];
    boxPoints[5][2] = range[5];

    boxPoints[6][0] = range[1];
    boxPoints[6][1] = range[3];
    boxPoints[6][2] = range[5];

    boxPoints[7][0] = range[1];
    boxPoints[7][1] = range[2];
    boxPoints[7][2] = range[5];

    for (int i = 0; i < 8; i++)
    {
      this->Transform(boxPoints[i]);
    }

    range[0] = range[2] = range[4] = VTK_DOUBLE_MAX;
    range[1] = range[3] = range[5] = -VTK_DOUBLE_MAX;

    for (int i = 0; i < 8; i++)
    {
      for (int j = 0; j < 3; j++)
      {
        if (boxPoints[i][j] < range[2 * j])
        {
          range[2 * j] = boxPoints[i][j];
        }
        if (boxPoints[i][j] > range[2 * j + 1])
        {
          range[2 * j + 1] = boxPoints[i][j];
        }
      }
    }
    if (finite)
    {
      this->InvalidFiniteRange = false;
    }
    else
    {
      this->InvalidRange = false;
    }
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::GetTuples(vtkIdList* ptIds, vtkAbstractArray* output)
{
  vtkDataArray* da = vtkDataArray::FastDownCast(output);
  if (!da)
  {
    vtkWarningMacro(<< "Input is not a vtkDataArray");
    return;
  }

  if (da->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkWarningMacro(<< "Incorrect number of components in input array.");
    return;
  }

  const vtkIdType numPoints = ptIds->GetNumberOfIds();
  double* tempData = new double[this->NumberOfComponents];
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    this->GetTuple(ptIds->GetId(i), tempData);
    da->SetTuple(i, tempData);
  }
  delete[] tempData;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray* output)
{
  vtkDataArray* da = vtkDataArray::FastDownCast(output);
  if (!da)
  {
    vtkErrorMacro(<< "Input is not a vtkDataArray");
    return;
  }

  if (da->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkErrorMacro(<< "Incorrect number of components in input array.");
    return;
  }

  double* tempData = new double[this->NumberOfComponents];
  for (vtkIdType daTupleId = 0; p1 <= p2; ++p1)
  {
    this->GetTuple(p1, tempData);
    da->SetTuple(daTupleId++, tempData);
  }
  delete[] tempData;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::Squeeze()
{
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkArrayIterator* vtkPeriodicDataArray<Scalar>::NewIterator()
{
  vtkArrayIteratorTemplate<Scalar>* iter = vtkArrayIteratorTemplate<Scalar>::New();
  iter->Initialize(this);
  return iter;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkPeriodicDataArray<Scalar>::LookupValue(vtkVariant)
{
  vtkErrorMacro("Lookup not implemented in this container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::LookupValue(vtkVariant, vtkIdList*)
{
  vtkErrorMacro("Lookup not implemented in this container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkVariant vtkPeriodicDataArray<Scalar>::GetVariantValue(vtkIdType idx)
{
  return vtkVariant(this->GetValueReference(idx));
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::ClearLookup()
{
  vtkErrorMacro("Lookup not implemented in this container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
double* vtkPeriodicDataArray<Scalar>::GetTuple(vtkIdType i)
{
  if (this->TempTupleIdx != i)
  {
    this->GetTypedTuple(i, this->TempScalarArray);
    this->TempTupleIdx = i;
  }
  for (int j = 0; j < this->NumberOfComponents; j++)
  {
    this->TempDoubleArray[j] = static_cast<double>(this->TempScalarArray[j]);
  }
  return this->TempDoubleArray;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::GetTuple(vtkIdType i, double* tuple)
{
  if (this->TempTupleIdx != i)
  {
    this->GetTypedTuple(i, this->TempScalarArray);
    this->TempTupleIdx = i;
  }
  for (int j = 0; j < this->NumberOfComponents; j++)
  {
    tuple[j] = static_cast<double>(this->TempScalarArray[j]);
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkPeriodicDataArray<Scalar>::LookupTypedValue(Scalar)
{
  vtkErrorMacro("Lookup not implemented in this container.");
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::LookupTypedValue(Scalar, vtkIdList*)
{
  vtkErrorMacro("Lookup not implemented in this container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
typename vtkPeriodicDataArray<Scalar>::ValueType vtkPeriodicDataArray<Scalar>::GetValue(
  vtkIdType idx) const
{
  return const_cast<vtkPeriodicDataArray<Scalar>*>(this)->GetValueReference(idx);
}

//------------------------------------------------------------------------------
template <class Scalar>
typename vtkPeriodicDataArray<Scalar>::ValueType& vtkPeriodicDataArray<Scalar>::GetValueReference(
  vtkIdType idx)
{
  vtkIdType tupleIdx = idx / this->NumberOfComponents;
  if (tupleIdx != this->TempTupleIdx)
  {
    this->GetTypedTuple(tupleIdx, this->TempScalarArray);
    this->TempTupleIdx = tupleIdx;
  }
  return this->TempScalarArray[idx % this->NumberOfComponents];
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::GetTypedTuple(vtkIdType tupleId, Scalar* tuple) const
{
  this->Data->GetTypedTuple(tupleId, tuple);
  this->Transform(tuple);
}

//------------------------------------------------------------------------------
template <class Scalar>
typename vtkPeriodicDataArray<Scalar>::ValueType vtkPeriodicDataArray<Scalar>::GetTypedComponent(
  vtkIdType tupleId, int compId) const
{
  if (tupleId != this->TempTupleIdx)
  {
    this->Data->GetTypedTuple(tupleId, this->TempScalarArray);
    this->Transform(const_cast<Scalar*>(this->TempScalarArray));
    *const_cast<vtkIdType*>(&this->TempTupleIdx) = tupleId;
  }

  return this->TempScalarArray[compId];
}

//------------------------------------------------------------------------------
template <class Scalar>
unsigned long int vtkPeriodicDataArray<Scalar>::GetActualMemorySize() const
{
  return static_cast<unsigned long int>(
    (this->NumberOfComponents * (sizeof(Scalar) + sizeof(double)) + sizeof(*this)) / 1024);
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkTypeBool vtkPeriodicDataArray<Scalar>::Allocate(vtkIdType, vtkIdType)
{
  vtkErrorMacro("Read only container.");
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkTypeBool vtkPeriodicDataArray<Scalar>::Resize(vtkIdType)
{
  vtkErrorMacro("Read only container.");
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::SetNumberOfTuples(vtkIdType)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::SetTuple(vtkIdType, vtkIdType, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::SetTuple(vtkIdType, const float*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::SetTuple(vtkIdType, const double*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InsertTuple(vtkIdType, vtkIdType, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InsertTuple(vtkIdType, const float*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InsertTuple(vtkIdType, const double*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InsertTuples(vtkIdList*, vtkIdList*, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InsertTuplesStartingAt(vtkIdType, vtkIdList*, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InsertTuples(vtkIdType, vtkIdType, vtkIdType, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkPeriodicDataArray<Scalar>::InsertNextTuple(vtkIdType, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkPeriodicDataArray<Scalar>::InsertNextTuple(const float*)
{
  vtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkPeriodicDataArray<Scalar>::InsertNextTuple(const double*)
{
  vtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::DeepCopy(vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::DeepCopy(vtkDataArray*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InterpolateTuple(
  vtkIdType, vtkIdList*, vtkAbstractArray*, double*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InterpolateTuple(
  vtkIdType, vtkIdType, vtkAbstractArray*, vtkIdType, vtkAbstractArray*, double)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::SetVariantValue(vtkIdType, vtkVariant)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InsertVariantValue(vtkIdType, vtkVariant)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::RemoveTuple(vtkIdType)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::RemoveFirstTuple()
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::RemoveLastTuple()
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::SetTypedTuple(vtkIdType, const Scalar*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::SetTypedComponent(vtkIdType, int, Scalar)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InsertTypedTuple(vtkIdType, const Scalar*)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkPeriodicDataArray<Scalar>::InsertNextTypedTuple(const Scalar*)
{
  vtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::SetValue(vtkIdType, Scalar)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkPeriodicDataArray<Scalar>::InsertNextValue(Scalar)
{
  vtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InsertValue(vtkIdType, Scalar)
{
  vtkErrorMacro("Read only container.");
}

//------------------------------------------------------------------------------
template <class Scalar>
bool vtkPeriodicDataArray<Scalar>::AllocateTuples(vtkIdType)
{
  vtkErrorMacro("Read only container.");
  return false;
}

//------------------------------------------------------------------------------
template <class Scalar>
bool vtkPeriodicDataArray<Scalar>::ReallocateTuples(vtkIdType)
{
  vtkErrorMacro("Read only container.");
  return false;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkPeriodicDataArray<Scalar>::InvalidateRange()
{
  this->InvalidRange = true;
}

//------------------------------------------------------------------------------
template <class Scalar>
void* vtkPeriodicDataArray<Scalar>::GetVoidPointer(vtkIdType valueIdx)
{
  if (!this->Cache)
  {
    this->Cache = vtkAOSDataArrayTemplate<Scalar>::New();
    this->Cache->DeepCopy(this);
  }
  return this->Cache->GetVoidPointer(valueIdx);
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkPeriodicDataArray<Scalar>::vtkPeriodicDataArray()
{
  this->NumberOfComponents = 0;
  this->TempScalarArray = nullptr;
  this->TempDoubleArray = nullptr;
  this->TempTupleIdx = -1;
  this->Data = nullptr;
  this->Cache = nullptr;
  this->MaxId = -1;
  this->Size = 0;

  this->InvalidRange = true;
  this->Normalize = false;
  this->PeriodicRange[0] = this->PeriodicRange[2] = this->PeriodicRange[4] = VTK_DOUBLE_MAX;
  this->PeriodicRange[1] = this->PeriodicRange[3] = this->PeriodicRange[5] = -VTK_DOUBLE_MAX;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkPeriodicDataArray<Scalar>::~vtkPeriodicDataArray()
{
  this->Initialize();
  if (this->Cache)
  {
    this->Cache->Delete();
    this->Cache = nullptr;
  }
}
VTK_ABI_NAMESPACE_END
