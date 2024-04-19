// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"

namespace
{

//----------------SetTuples (from array+vtkIdList to range)---------------------
struct SetTuplesIdListRangeWorker
{
  vtkIdList* SrcTuples;
  vtkIdType DstStartTuple;

  SetTuplesIdListRangeWorker(vtkIdList* srcTuples, vtkIdType dstStartTuple)
    : SrcTuples(srcTuples)
    , DstStartTuple(dstStartTuple)
  {
  }

  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT* src, DstArrayT* dst) const
  {
    const auto srcTuples = vtk::DataArrayTupleRange(src);
    auto dstTuples = vtk::DataArrayTupleRange(dst);

    vtkIdType numTuples = this->SrcTuples->GetNumberOfIds();
    vtkIdType dstT = this->DstStartTuple;

    for (vtkIdType t = 0; t < numTuples; ++t)
    {
      vtkIdType srcT = this->SrcTuples->GetId(t);
      dstTuples[dstT + t] = srcTuples[srcT];
    }
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
void vtkDataArray::InsertTuplesStartingAt(
  vtkIdType dstStart, vtkIdList* srcIds, vtkAbstractArray* src)
{
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

  vtkIdType maxSrcTupleId = srcIds->GetId(0);
  vtkIdType maxDstTupleId = dstStart + srcIds->GetNumberOfIds() - 1;
  for (vtkIdType i = 1; i < srcIds->GetNumberOfIds(); ++i)
  {
    maxSrcTupleId = std::max(maxSrcTupleId, srcIds->GetId(i));
  }

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

  SetTuplesIdListRangeWorker worker(srcIds, dstStart);
  if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(srcDA, this, worker))
  {
    worker(srcDA, this);
  }
}
VTK_ABI_NAMESPACE_END
