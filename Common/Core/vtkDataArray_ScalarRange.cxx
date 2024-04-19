// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"

namespace
{

// Wrap the DoCompute[Scalar|Vector]Range calls for vtkArrayDispatch:
struct ScalarRangeDispatchWrapper
{
  bool Success;
  double* Range;
  const unsigned char* GhostArray;
  unsigned char GhostTypesToSkip;

  ScalarRangeDispatchWrapper(
    double* range, const unsigned char* ghostArray, unsigned char ghostTypesToSkip)
    : Success(false)
    , Range(range)
    , GhostArray(ghostArray)
    , GhostTypesToSkip(ghostTypesToSkip)
  {
  }

  template <typename ArrayT>
  void operator()(ArrayT* array)
  {
    this->Success = vtkDataArrayPrivate::DoComputeScalarRange(array, this->Range,
      vtkDataArrayPrivate::AllValues(), this->GhostArray, this->GhostTypesToSkip);
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
bool vtkDataArray::ComputeScalarRange(double* ranges)
{
  return this->ComputeScalarRange(ranges, nullptr);
}

//------------------------------------------------------------------------------
bool vtkDataArray::ComputeScalarRange(
  double* ranges, const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  ScalarRangeDispatchWrapper worker(ranges, ghosts, ghostsToSkip);
  if (!vtkArrayDispatch::Dispatch::Execute(this, worker))
  {
    worker(this);
  }
  return worker.Success;
}
VTK_ABI_NAMESPACE_END
