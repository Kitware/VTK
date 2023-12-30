// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArray.h"

#include "vtkArrayDispatch.h"

namespace
{

// Wrap the DoCompute[Scalar|Vector]Range calls for vtkArrayDispatch:
struct FiniteVectorRangeDispatchWrapper
{
  bool Success;
  double* Range;
  const unsigned char* GhostArray;
  unsigned char GhostTypesToSkip;

  FiniteVectorRangeDispatchWrapper(
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
    this->Success = vtkDataArrayPrivate::DoComputeVectorRange(array, this->Range,
      vtkDataArrayPrivate::FiniteValues(), this->GhostArray, this->GhostTypesToSkip);
  }
};

} // end anon namespace

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
bool vtkDataArray::ComputeFiniteVectorRange(double range[2])
{
  return this->ComputeFiniteVectorRange(range, nullptr);
}

//------------------------------------------------------------------------------
bool vtkDataArray::ComputeFiniteVectorRange(
  double range[2], const unsigned char* ghosts, unsigned char ghostsToSkip)
{
  FiniteVectorRangeDispatchWrapper worker(range, ghosts, ghostsToSkip);
  if (!vtkArrayDispatch::Dispatch::Execute(this, worker))
  {
    worker(this);
  }
  return worker.Success;
}
VTK_ABI_NAMESPACE_END
