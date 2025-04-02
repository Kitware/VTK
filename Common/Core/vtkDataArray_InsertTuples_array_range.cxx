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
#define VTK_COPY_TUPLES(NUM_COMPONENTS)                                                            \
  case NUM_COMPONENTS:                                                                             \
  {                                                                                                \
    const auto srcTuples = vtk::DataArrayTupleRange<NUM_COMPONENTS>(                               \
      src, this->SrcStartTuple, this->SrcStartTuple + this->NumTuples);                            \
    auto dstTuples = vtk::DataArrayTupleRange<NUM_COMPONENTS>(                                     \
      dst, this->DstStartTuple, this->DstStartTuple + this->NumTuples);                            \
    std::copy(srcTuples.cbegin(), srcTuples.cend(), dstTuples.begin());                            \
    break;                                                                                         \
  }
    switch (src->GetNumberOfComponents())
    {
      VTK_COPY_TUPLES(1)
      VTK_COPY_TUPLES(2)
      VTK_COPY_TUPLES(3)
      default:
        VTK_COPY_TUPLES(vtk::detail::DynamicTupleSize)
    }
#undef VTK_COPY_TUPLES
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
