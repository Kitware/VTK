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

//-----------------------------------------------------------------------------
template<class D, class S, class T, class I, class SR>
void vtkAgnosticArray<D,S,T,I,SR>::
InsertTuples(vtkIdList *dstIds, vtkIdList *srcIds, vtkAbstractArray *source)
{
  if (this->GetDataType() != source->GetDataType())
    {
    vtkErrorMacro("Input and output array data types do not match.");
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
  if (!this->EnsureAccess(maxDstId))
    {
    vtkErrorMacro("Failed to allocate memory.");
    return;
    }

  //// Copy directly into our array if the source has supporting API:
  //if (vtkTypedDataArray<T> *typedSource =
  //    vtkTypedDataArray<T>::FastDownCast(source))
  //  {
  //  for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
  //    {
  //    typedSource->GetTupleValue(srcIds->GetId(idIndex),
  //                               this->GetPointer(dstIds->GetId(idIndex)
  //                                                * this->NumberOfComponents));
  //    }
  //  }
  //else if (vtkDataArray *dataSource = vtkDataArray::FastDownCast(source))
  //  {
  //  // Otherwise use the double interface
  //  for (vtkIdType idIndex = 0; idIndex < numIds; ++idIndex)
  //    {
  //    this->SetTuple(dstIds->GetId(idIndex),
  //                   dataSource->GetTuple(srcIds->GetId(idIndex)));
  //    }
  //  }
  //else
  //  {
  //  vtkWarningMacro("Input array is not a vtkDataArray subclass!");
  //  return;
  //  }

  //vtkIdType maxId = maxSize - 1;
  //if (maxId > this->MaxId)
  //  {
  //  this->MaxId = maxId;
  //  }

  //this->DataChanged();
}

//-----------------------------------------------------------------------------
template<class D, class S, class T, class I, class SR>
void vtkAgnosticArray<D,S,T,I,SR>::
InsertTuples(vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source)
{
}
