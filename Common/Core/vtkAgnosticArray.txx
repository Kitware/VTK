/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAgnosticArray.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkAgnosticArrayHelpers.h"
#include "vtkVariantCast.h"

#define vtkAgnosticArrayT(returnType) \
  template <class D, class S, class T, class TI, class SR> \
  returnType vtkAgnosticArray<D, S, T, TI, SR>

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::InsertTuples(
  vtkIdList *dstIds, vtkIdList *srcIds, vtkAbstractArray *source)
{
  // XXX Should these be implemented in vtkDataArray?
  if (this->GetDataType() != source->GetDataType())
    {
    vtkErrorMacro("Input and output array data types do not match.");
    return;
    }
  if (vtkDataArray::SafeDownCast(source) == NULL)
    {
    vtkErrorMacro("Input array is not a vtkDataArray subclass!");
    return;
    }
  if (this->NumberOfComponents != source->GetNumberOfComponents())
    {
    vtkErrorMacro("Input and output component sizes do not match.");
    return;
    }

  vtkIdType numIds = dstIds->GetNumberOfIds();
  if (srcIds->GetNumberOfIds() != numIds)
    {
    vtkErrorMacro("Input and output id array sizes do not match.");
    return;
    }

  // Find maximum destination id and resize if needed
  vtkIdType maxDstId = 0;
  for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
    {
    maxDstId = std::max(maxDstId, dstIds->GetId(idIndex));
    }
  if (!this->EnsureAccessToTuple(maxDstId))
    {
    vtkErrorMacro("Failed to allocate memory.");
    return;
    }

  for (vtkIdType cc=0; cc < numIds; ++cc)
    {
    vtkAgnosticArrayHelpers::SetTuple(this, dstIds->GetId(cc), source, srcIds->GetId(cc));
    }
  this->DataChanged();
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::InsertTuples(vtkIdType dstStart, vtkIdType n,
  vtkIdType srcStart, vtkAbstractArray* source)
{
  // XXX Should these be implemented in vtkDataArray?
  if (n < 0)
    {
    vtkErrorMacro("Number of tuples to insert cannot be negative!");
    return;
    }
  if (this->GetDataType() != source->GetDataType())
    {
    vtkErrorMacro("Input and output array data types do not match.");
    return;
    }
  if (vtkDataArray::SafeDownCast(source) == NULL)
    {
    vtkErrorMacro("Input array is not a vtkDataArray subclass!");
    return;
    }
  if (this->NumberOfComponents != source->GetNumberOfComponents())
    {
    vtkErrorMacro("Input and output component sizes do not match.");
    return;
    }
  if (source->GetNumberOfTuples() < (srcStart + n))
    {
    vtkErrorMacro("Input does not have enough data to copy from. "
      << "Expected at least " << (srcStart+n) << " tuples, got "
      << source->GetNumberOfTuples() << " tuples.");
    return;
    }

  // Find maximum destination id and resize if needed
  if (!this->EnsureAccessToTuple(dstStart+n-1))
    {
    vtkErrorMacro("Failed to allocate memory.");
    return;
    }

  for (vtkIdType cc=0; cc < n; ++cc)
    {
    vtkAgnosticArrayHelpers::SetTuple(this, dstStart + cc, source, srcStart + cc);
    }
  this->DataChanged();
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(double *)::GetTuple(vtkIdType i)
{
  // XXX Should these be implemented in vtkDataArray?
  this->LegacyTuple.resize(this->GetNumberOfComponents());
  vtkAgnosticArrayHelpers::GetTuple(this, i,  &this->LegacyTuple[0]);
  return &this->LegacyTuple[0];
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::GetTuple(vtkIdType i, double * tuple)
{
  // XXX Should these be implemented in vtkDataArray?
  vtkAgnosticArrayHelpers::GetTuple(this, i, tuple);
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray *source)
{
  // XXX Should these be implemented in vtkDataArray?
  vtkAgnosticArrayHelpers::SetTuple(this, i, source, j);
  this->DataChanged();
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::SetTuple(vtkIdType i, const float *source)
{
  // XXX Should these be implemented in vtkDataArray?
  for (int cc=0, max=this->GetNumberOfComponents(); cc < max; ++cc)
    {
    this->Begin(i)[cc] = static_cast<ScalarType>(source[cc]);
    }
  this->DataChanged();
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::SetTuple(vtkIdType i, const double *source)
{
  // XXX Should these be implemented in vtkDataArray?
  for (int cc=0, max=this->GetNumberOfComponents(); cc < max; ++cc)
    {
    this->Begin(i)[cc] = static_cast<ScalarType>(source[cc]);
    }
  this->DataChanged();
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::RemoveTuple(vtkIdType id)
{
  if (id < 0 || id >= this->GetNumberOfTuples())
    {
    // Nothing to be done
    return;
    }
  if (id == (this->GetNumberOfTuples() - 1))
    {
    // To remove last item, just decrease the size by one
    this->RemoveLastTuple();
    return;
    }

  // This is a very slow implementation since it uses generic API. Subclasses
  // are encouraged to provide a faster implementation.
  const vtkIdType len = (this->GetNumberOfTuples() - id) - 1;
  assert(len > 0);

  int numComps = this->GetNumberOfComponents();
  TupleIteratorType from = this->Begin(id+1);
  TupleIteratorType to = this->Begin(id);
  TupleIteratorType end = this->End();
  for (; from != end; ++to, ++from)
    {
    for (int cc=0; cc < numComps; ++cc)
      {
      to[cc] = from[cc];
      }
    }
  this->SetNumberOfTuples(this->GetNumberOfTuples() - 1);
  this->DataChanged();
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::SetVoidArray(void*, vtkIdType, int)
{
  vtkErrorMacro("SetVoidArray is not supported by this class.");
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void*)::WriteVoidPointer(vtkIdType id, vtkIdType number)
{
  vtkErrorMacro("WriteVoidPointer is not supported by this class.");
  return NULL;
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void*)::GetVoidPointer(vtkIdType)
{
  vtkErrorMacro("GetVoidPointer is not supported by this class.");
  return NULL;
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(vtkIdType)::LookupValue(vtkVariant valueVariant)
{
  bool valid = true;
  ScalarType value = vtkVariantCast<ScalarType>(valueVariant, &valid);
  if (valid)
    {
    return this->LookupTypedValue(value);
    }
  return -1;
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(vtkIdType)::LookupTypedValue(ScalarType value)
{
  return this->Lookup.LookupValue(value);
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::LookupValue(vtkVariant valueVariant, vtkIdList* ids)
{
  ids->Reset();
  bool valid = true;
  ScalarType value = vtkVariantCast<ScalarType>(valueVariant, &valid);
  if (valid)
    {
    this->LookupTypedValue(value, ids);
    }
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::LookupTypedValue(ScalarType value, vtkIdList* ids)
{
  ids->Reset();
  this->Lookup.LookupValue(value, ids);
}

//-----------------------------------------------------------------------------
vtkAgnosticArrayT(void)::SetVariantValue(vtkIdType idx, vtkVariant valueVariant)
{
  bool valid = true;
  ScalarType value = vtkVariantCast<ScalarType>(valueVariant, &valid);
  if (valid)
    {
    vtkIdType tupleIndex = static_cast<vtkIdType>(idx / this->NumberOfComponents);
    int comp = static_cast<int>(idx % this->NumberOfComponents);
    this->Begin(tupleIndex)[comp] = value;
    }
}

#undef vtkAgnosticArrayT
