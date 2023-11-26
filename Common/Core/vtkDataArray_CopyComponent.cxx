// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"

namespace
{

struct CopyComponentWorker
{
  CopyComponentWorker(int srcComponent, int dstComponent)
    : SourceComponent(srcComponent)
    , DestinationComponent(dstComponent)
  {
  }

  template <typename ArraySrc, typename ArrayDst>
  void operator()(ArraySrc* dst, ArrayDst* src) const
  {
    const auto srcRange = vtk::DataArrayTupleRange(src);
    auto dstRange = vtk::DataArrayTupleRange(dst);

    using DstType = vtk::GetAPIType<ArrayDst>;
    auto dstIter = dstRange.begin();

    for (auto v : srcRange)
    {
      (*dstIter)[DestinationComponent] = static_cast<DstType>(v[SourceComponent]);
      ++dstIter;
    }
  }

  int SourceComponent = 0;
  int DestinationComponent = 0;
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
void vtkDataArray::CopyComponent(int dstComponent, vtkDataArray* src, int srcComponent)
{
  if (this->GetNumberOfTuples() != src->GetNumberOfTuples())
  {
    vtkErrorMacro(<< "Number of tuples in 'from' (" << src->GetNumberOfTuples() << ") and 'to' ("
                  << this->GetNumberOfTuples() << ") do not match.");
    return;
  }

  if (dstComponent < 0 || dstComponent >= this->GetNumberOfComponents())
  {
    vtkErrorMacro(<< "Specified component " << dstComponent << " in 'to' array is not in [0, "
                  << this->GetNumberOfComponents() << ")");
    return;
  }

  if (srcComponent < 0 || srcComponent >= src->GetNumberOfComponents())
  {
    vtkErrorMacro(<< "Specified component " << srcComponent << " in 'from' array is not in [0, "
                  << src->GetNumberOfComponents() << ")");
    return;
  }

  CopyComponentWorker copyComponentWorker(srcComponent, dstComponent);
  if (!vtkArrayDispatch::Dispatch2::Execute(this, src, copyComponentWorker))
  {
    copyComponentWorker(this, src);
  }
}
VTK_ABI_NAMESPACE_END
