/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArrayTemplateHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataArrayTemplateHelper.h"

#include "vtkDataArrayIteratorMacro.h"
#include "vtkDataArrayTemplate.h"

//------------------------------------------------------------------------------
void vtkDataArrayTemplateHelper::InsertTuples(
    vtkDataArray *dst, vtkIdType dstStart, vtkIdType n, vtkIdType srcStart,
    vtkAbstractArray *source)
{
  vtkIdType srcEnd = srcStart + n;
  if (srcEnd > source->GetNumberOfTuples())
    {
    vtkWarningWithObjectMacro(dst,
                              "Source range exceeds array size (srcStart="
                              << srcStart << ", n=" << n << ", numTuples="
                              << source->GetNumberOfTuples() << ").");
    return;
    }

  // Find maximum destination id and resize if needed
  vtkIdType dstEnd = dstStart + n;
  vtkIdType maxSize = dstEnd * dst->GetNumberOfComponents();
  if (maxSize > dst->GetSize())
    {
    void *resizeResult(NULL);
    switch (dst->GetDataType())
      {
      vtkTemplateMacro(resizeResult =
        static_cast<vtkDataArrayTemplate<VTK_TT>*>(dst)->ResizeAndExtend(maxSize);
      );
      }
    if (resizeResult == NULL)
      {
      vtkWarningWithObjectMacro(dst, "Failed to allocate memory.");
      return;
      }
    }

  // TypedDataArrays and their subclasses have iterator interfaces:
  if (source->GetArrayType() == vtkAbstractArray::TypedDataArray)
    {
    switch (source->GetDataType())
      {
      vtkDataArrayIteratorMacro(
            source,
            std::copy(vtkDABegin + srcStart, vtkDABegin + srcEnd,
                      static_cast<vtkDataArrayTemplate<vtkDAValueType>*>(dst)
                      ->Begin() + dstStart)
            );
      }
    }
  else if (vtkDataArray *dataSource = vtkDataArray::FastDownCast(source))
    {
    // Otherwise use the double interface
    for (vtkIdType i = 0; i < n; ++i)
      {
      dst->SetTuple(dstStart + i, dataSource->GetTuple(srcStart + i));
      }
    }
  else
    {
    vtkWarningWithObjectMacro(dst,
                              "Input array is not a vtkDataArray subclass!");
    return;
    }

  vtkIdType maxId = maxSize - 1;
  if (maxId > dst->MaxId)
    {
    dst->MaxId = maxId;
    }

  dst->DataChanged();
}
