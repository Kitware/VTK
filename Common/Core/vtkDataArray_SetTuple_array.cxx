// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"

namespace
{

//----------------SetTuple (from array)-----------------------------------------
struct SetTupleArrayWorker
{
  vtkIdType SrcTuple;
  vtkIdType DstTuple;

  SetTupleArrayWorker(vtkIdType srcTuple, vtkIdType dstTuple)
    : SrcTuple(srcTuple)
    , DstTuple(dstTuple)
  {
  }

  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT* src, DstArrayT* dst) const
  {
    const auto srcTuples = vtk::DataArrayTupleRange(src);
    auto dstTuples = vtk::DataArrayTupleRange(dst);

    dstTuples[this->DstTuple] = srcTuples[this->SrcTuple];
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
void vtkDataArray::SetTuple(vtkIdType dstTupleIdx, vtkIdType srcTupleIdx, vtkAbstractArray* source)
{
  vtkDataArray* srcDA = vtkDataArray::FastDownCast(source);
  if (!srcDA)
  {
    vtkErrorMacro(
      "Source array must be a vtkDataArray subclass (got " << source->GetClassName() << ").");
    return;
  }

  if (source->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkErrorMacro("Number of components do not match: Source: "
      << source->GetNumberOfComponents() << " Dest: " << this->GetNumberOfComponents());
    return;
  }

  SetTupleArrayWorker worker(srcTupleIdx, dstTupleIdx);
  if (!vtkArrayDispatch::Dispatch2::Execute(srcDA, this, worker))
  {
    worker(srcDA, this);
  }
}
VTK_ABI_NAMESPACE_END
