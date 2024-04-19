// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkToAffineArrayStrategy.h"

#include "vtkAffineArray.h"
#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

namespace
{

using Dispatch = vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AllArrays>;

template <typename T>
struct AffineCheckerWorklet
{
  bool IsAffine = true;
  T Difference;
  double Tolerance;

  AffineCheckerWorklet(T diff, double tol)
    : Difference(diff)
    , Tolerance(tol)
  {
  }

  template <typename Iterator>
  void operator()(Iterator begin, Iterator end)
  {
    for (auto it = begin; it != end; ++it)
    {
      if (std::abs(static_cast<double>((*(it + 1) - *it) - this->Difference)) > this->Tolerance)
      {
        this->IsAffine = false;
        return;
      }
    }
  }
};

struct AffineChecker
{
  template <typename ArrayT>
  void operator()(ArrayT* arr, double tol, bool& isAffine)
  {
    using VType = vtk::GetAPIType<ArrayT>;

    auto range = vtk::DataArrayValueRange(arr);
    if (range.size() < 3)
    {
      isAffine = true;
      return;
    }
    VType diff = range[1] - range[0];
    AffineCheckerWorklet<VType> worker(diff, tol);
    vtkSMPTools::For(range.begin() + 1, range.end() - 1, worker);
    isAffine = worker.IsAffine;
  }
};

struct AffineGenerator
{
  template <typename ArrayT>
  void operator()(ArrayT* arr, vtkSmartPointer<vtkDataArray>& target)
  {
    using VType = vtk::GetAPIType<ArrayT>;

    auto range = vtk::DataArrayValueRange(arr);
    VType intercept = range[0];
    VType slope = 0;
    if (range.size() != 1)
    {
      slope = range[1] - range[0];
    }
    vtkNew<vtkAffineArray<VType>> affine;
    affine->SetBackend(std::make_shared<vtkAffineImplicitBackend<VType>>(slope, intercept));
    affine->SetNumberOfComponents(arr->GetNumberOfComponents());
    affine->SetNumberOfTuples(arr->GetNumberOfTuples());
    affine->SetName(arr->GetName());
    target = affine;
  }
};

}

VTK_ABI_NAMESPACE_BEGIN
//-------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkToAffineArrayStrategy);

//-------------------------------------------------------------------------
void vtkToAffineArrayStrategy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << std::flush;
}

//-------------------------------------------------------------------------
vtkToImplicitStrategy::Optional vtkToAffineArrayStrategy::EstimateReduction(vtkDataArray* arr)
{
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to affine array.");
    return vtkToImplicitStrategy::Optional();
  }
  vtkIdType nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return vtkToImplicitStrategy::Optional();
  }
  ::AffineChecker checker;
  bool isAffine = false;
  if (!Dispatch::Execute(arr, checker, this->Tolerance, isAffine))
  {
    checker(arr, this->Tolerance, isAffine);
  }
  return isAffine ? vtkToImplicitStrategy::Optional(2.0 / nVals)
                  : vtkToImplicitStrategy::Optional();
}

//-------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkToAffineArrayStrategy::Reduce(vtkDataArray* arr)
{
  vtkSmartPointer<vtkDataArray> res = nullptr;
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to affine array.");
    return res;
  }
  vtkIdType nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return res;
  }
  ::AffineGenerator generator;
  if (!Dispatch::Execute(arr, generator, res))
  {
    generator(arr, res);
  }
  return res;
}

VTK_ABI_NAMESPACE_END
