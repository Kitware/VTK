// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"

namespace
{

//----------------SetTuples (from array+range)----------------------------------
struct SetTuplesRangeWorker
{
  vtkIdType SrcStartTuple;
  vtkIdType DstStartTuple;
  vtkIdType NumTuples;

  SetTuplesRangeWorker(vtkIdType srcStartTuple, vtkIdType dstStartTuple, vtkIdType numTuples)
    : SrcStartTuple(srcStartTuple)
    , DstStartTuple(dstStartTuple)
    , NumTuples(numTuples)
  {
  }

  // Generic implementation. We perform the obvious optimizations for AOS/SOA
  // in the derived class implementations.
  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT* src, DstArrayT* dst) const
  {
    const auto srcTuples = vtk::DataArrayTupleRange(src);
    auto dstTuples = vtk::DataArrayTupleRange(dst);

    vtkIdType srcT = this->SrcStartTuple;
    vtkIdType srcTEnd = srcT + this->NumTuples;
    vtkIdType dstT = this->DstStartTuple;

    while (srcT < srcTEnd)
    {
      dstTuples[dstT++] = srcTuples[srcT++];
    }
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
void vtkDataArray::InsertTuples(
  vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* src)
{
  if (n == 0)
  {
    return;
  }
  if (src->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkErrorMacro("Number of components do not match: Source: "
      << src->GetNumberOfComponents() << " Dest: " << this->GetNumberOfComponents());
    return;
  }
  vtkDataArray* srcDA = vtkDataArray::FastDownCast(src);
  if (!srcDA)
  {
    vtkErrorMacro("Source array must be a subclass of vtkDataArray. Got: " << src->GetClassName());
    return;
  }

  vtkIdType maxSrcTupleId = srcStart + n - 1;
  vtkIdType maxDstTupleId = dstStart + n - 1;

  if (maxSrcTupleId >= src->GetNumberOfTuples())
  {
    vtkErrorMacro("Source array too small, requested tuple at index "
      << maxSrcTupleId << ", but there are only " << src->GetNumberOfTuples()
      << " tuples in the array.");
    return;
  }

  vtkIdType newSize = (maxDstTupleId + 1) * this->NumberOfComponents;
  if (this->Size < newSize)
  {
    if (!this->Resize(maxDstTupleId + 1))
    {
      vtkErrorMacro("Resize failed.");
      return;
    }
  }

  this->MaxId = std::max(this->MaxId, newSize - 1);

  SetTuplesRangeWorker worker(srcStart, dstStart, n);
  if (!vtkArrayDispatch::Dispatch2::Execute(srcDA, this, worker))
  {
    worker(srcDA, this);
  }
}
VTK_ABI_NAMESPACE_END
