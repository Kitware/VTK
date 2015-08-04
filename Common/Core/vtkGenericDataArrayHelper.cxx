/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataArrayHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericDataArrayHelper.h"

#include "vtkArrayDispatch.h"
#include "vtkObjectFactory.h"
#include "vtkSoADataArrayTemplate.h"
#include "vtkAoSDataArrayTemplate.h"

//============================================================================
namespace
{

//--------------------------------------------------------------------------
struct SetTupleWorker
{
  vtkIdType SourceTuple;
  vtkIdType DestTuple;

  SetTupleWorker(vtkIdType sourceTuple, vtkIdType destTuple)
    : SourceTuple(sourceTuple),
      DestTuple(destTuple)
  {}

  template <typename ArrayIn, typename ArrayOut>
  void operator()(ArrayIn *source, ArrayOut *dest)
  {
    for (int cc = 0, max = dest->GetNumberOfComponents(); cc < max; ++cc)
      {
      dest->SetComponentValue(this->DestTuple, cc,
        static_cast<typename ArrayOut::ValueType>(
        source->GetComponentValue(this->SourceTuple, cc)));
      }
  }
};

//--------------------------------------------------------------------------
struct GetTupleWorker
{
  vtkIdType Tuple;
  double *Buffer;

  GetTupleWorker(vtkIdType tuple, double *buffer) : Tuple(tuple), Buffer(buffer)
  {}

  template <typename ArrayT>
  void operator()(ArrayT *source)
  {
    for (int cc = 0, max = source->GetNumberOfComponents(); cc < max; ++cc)
      {
      this->Buffer[cc] =
          static_cast<double>(source->GetComponentValue(this->Tuple, cc));
      }
  }
};

} // end anon namespace
//============================================================================


//----------------------------------------------------------------------------
vtkGenericDataArrayHelper::vtkGenericDataArrayHelper()
{
}

//----------------------------------------------------------------------------
vtkGenericDataArrayHelper::~vtkGenericDataArrayHelper()
{
}

//----------------------------------------------------------------------------
void vtkGenericDataArrayHelper::SetTuple(
  vtkAbstractArray* dest, vtkIdType destTuple,
  vtkAbstractArray* source, vtkIdType sourceTuple)
{
  if (!vtkDataTypesCompare(source->GetDataType(), dest->GetDataType()))
    {
    vtkGenericWarningMacro("Input and output array data types do not match.");
    return;
    }
  if (dest->GetNumberOfComponents() != source->GetNumberOfComponents())
    {
    vtkGenericWarningMacro("Input and output component sizes do not match.");
    return;
    }

  vtkDataArray *srcDA = vtkDataArray::SafeDownCast(source);
  vtkDataArray *dstDA = vtkDataArray::SafeDownCast(dest);
  if (!srcDA || !dstDA)
    {
    vtkGenericWarningMacro("This method expects both arrays to be vtkDataArray "
                           "subclasses.");
    return;
    }

  SetTupleWorker worker(sourceTuple, destTuple);
  vtkArrayDispatch::Dispatch2SameValueType::Execute(srcDA, dstDA, worker);
}

//----------------------------------------------------------------------------
void vtkGenericDataArrayHelper::GetTuple(vtkAbstractArray* source,
                                         vtkIdType tuple, double* buffer)
{
  vtkDataArray *srcDA = vtkDataArray::SafeDownCast(source);
  if (!srcDA)
    {
    vtkGenericWarningMacro("This method expects source to be a vtkDataArray "
                           "subclass.");
    return;
    }

  GetTupleWorker worker(tuple, buffer);
  vtkArrayDispatch::Dispatch::Execute(srcDA, worker);
}

//----------------------------------------------------------------------------
void vtkGenericDataArrayHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
