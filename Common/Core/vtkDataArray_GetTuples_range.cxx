// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"

namespace
{

//-----------------GetTuples (tuple range)--------------------------------------
struct GetTuplesRangeWorker
{
  vtkIdType Start;
  vtkIdType End; // Note that End is inclusive.

  GetTuplesRangeWorker(vtkIdType start, vtkIdType end)
    : Start(start)
    , End(end)
  {
  }

  template <typename Array1T, typename Array2T>
  void operator()(Array1T* src, Array2T* dst) const
  {
    const auto srcTuples = vtk::DataArrayTupleRange(src);
    auto dstTuples = vtk::DataArrayTupleRange(dst);

    for (vtkIdType srcT = this->Start, dstT = 0; srcT <= this->End; ++srcT, ++dstT)
    {
      dstTuples[dstT] = srcTuples[srcT];
    }
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
void vtkDataArray::GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray* aa)
{
  vtkDataArray* da = vtkDataArray::FastDownCast(aa);
  if (!da)
  {
    vtkWarningMacro("Input is not a vtkDataArray.");
    return;
  }

  if ((da->GetNumberOfComponents() != this->GetNumberOfComponents()))
  {
    vtkErrorMacro("Number of components for input and output do not match.\n"
                  "Source: "
      << this->GetNumberOfComponents()
      << "\n"
         "Destination: "
      << da->GetNumberOfComponents());
    return;
  }

  GetTuplesRangeWorker worker(p1, p2);
  if (!vtkArrayDispatch::Dispatch2::Execute(this, da, worker))
  {
    // Use fallback if dispatch fails.
    worker(this, da);
  }
}
VTK_ABI_NAMESPACE_END
