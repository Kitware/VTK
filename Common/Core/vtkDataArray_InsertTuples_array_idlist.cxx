// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"

namespace
{

//----------------SetTuples (from array+vtkIdList)------------------------------
struct SetTuplesIdListWorker
{
  vtkIdList* SrcTuples;
  vtkIdList* DstTuples;

  SetTuplesIdListWorker(vtkIdList* srcTuples, vtkIdList* dstTuples)
    : SrcTuples(srcTuples)
    , DstTuples(dstTuples)
  {
  }

  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT* src, DstArrayT* dst) const
  {
    const auto srcTuples = vtk::DataArrayTupleRange(src);
    auto dstTuples = vtk::DataArrayTupleRange(dst);

    vtkIdType numTuples = this->SrcTuples->GetNumberOfIds();
    for (vtkIdType t = 0; t < numTuples; ++t)
    {
      vtkIdType srcT = this->SrcTuples->GetId(t);
      vtkIdType dstT = this->DstTuples->GetId(t);

      dstTuples[dstT] = srcTuples[srcT];
    }
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
void vtkDataArray::InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* src)
{
  if (dstIds->GetNumberOfIds() == 0)
  {
    return;
  }
  if (dstIds->GetNumberOfIds() != srcIds->GetNumberOfIds())
  {
    vtkErrorMacro("Mismatched number of tuples ids. Source: "
      << srcIds->GetNumberOfIds() << " Dest: " << dstIds->GetNumberOfIds());
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

  vtkIdType maxSrcTupleId = srcIds->GetId(0);
  vtkIdType maxDstTupleId = dstIds->GetId(0);
  for (vtkIdType i = 1; i < dstIds->GetNumberOfIds(); ++i)
  {
    maxSrcTupleId = std::max(maxSrcTupleId, srcIds->GetId(i));
    maxDstTupleId = std::max(maxDstTupleId, dstIds->GetId(i));
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

  SetTuplesIdListWorker worker(srcIds, dstIds);
  if (!vtkArrayDispatch::Dispatch2::Execute(srcDA, this, worker))
  {
    worker(srcDA, this);
  }
}
VTK_ABI_NAMESPACE_END
