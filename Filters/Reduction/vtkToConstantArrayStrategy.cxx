// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkToConstantArrayStrategy.h"

#include "vtkArrayDispatch.h"
#include "vtkConstantArray.h"
#include "vtkDataArrayRange.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

namespace
{
using Dispatch = vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AllArrays>;

template <typename ValueType>
struct ThreadedCheckingWorklet
{

  bool IsConstant = true;
  ValueType Value;
  double Tolerance;

  ThreadedCheckingWorklet(ValueType val, double tol)
    : Value(val)
    , Tolerance(tol)
  {
  }

  template <class Iterator>
  void operator()(Iterator begin, Iterator end)
  {
    for (auto it = begin; it != end; ++it)
    {
      if (std::abs(static_cast<double>(*it - this->Value)) > this->Tolerance)
      {
        this->IsConstant = false;
        break;
      }
    }
  }
};

struct CheckConstantWorklet
{
  template <typename ArrayT>
  void operator()(ArrayT* arr, double tol, bool& isConstant) const
  {
    using VType = vtk::GetAPIType<ArrayT>;
    auto range = vtk::DataArrayValueRange(arr);

    if (!range.size())
    {
      isConstant = false;
      return;
    }

    ThreadedCheckingWorklet<VType> tcWorker(range[0], tol);
    vtkSMPTools::For(range.begin(), range.end(), tcWorker);
    isConstant = tcWorker.IsConstant;
  }
};

struct GenerateConstantWorklet
{
  template <typename ArrayT>
  void operator()(ArrayT* arr, vtkSmartPointer<vtkDataArray>& cArr) const
  {
    using VType = vtk::GetAPIType<ArrayT>;
    vtkNew<vtkConstantArray<VType>> constant;
    auto range = vtk::DataArrayValueRange(arr);
    constant->SetBackend(std::make_shared<vtkConstantImplicitBackend<VType>>(range[0]));
    constant->SetNumberOfComponents(arr->GetNumberOfComponents());
    constant->SetNumberOfTuples(arr->GetNumberOfTuples());
    constant->SetName(arr->GetName());
    cArr = constant;
  }
};
}

VTK_ABI_NAMESPACE_BEGIN
//-------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkToConstantArrayStrategy);

//-------------------------------------------------------------------------
void vtkToConstantArrayStrategy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << std::flush;
}

//-------------------------------------------------------------------------
vtkToImplicitStrategy::Optional vtkToConstantArrayStrategy::EstimateReduction(vtkDataArray* arr)
{
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to constant array.");
    return vtkToImplicitStrategy::Optional();
  }
  vtkIdType nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return vtkToImplicitStrategy::Optional();
  }
  bool isConstant = false;
  ::CheckConstantWorklet worker;
  if (!::Dispatch::Execute(arr, worker, this->Tolerance, isConstant))
  {
    worker(arr, this->Tolerance, isConstant);
  }
  return isConstant ? vtkToImplicitStrategy::Optional(1.0 / nVals)
                    : vtkToImplicitStrategy::Optional();
}

//-------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkToConstantArrayStrategy::Reduce(vtkDataArray* arr)
{
  vtkSmartPointer<vtkDataArray> res = nullptr;
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to constant array.");
    return res;
  }
  vtkIdType nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return res;
  }
  ::GenerateConstantWorklet worker;
  if (!::Dispatch::Execute(arr, worker, res))
  {
    worker(arr, res);
  }
  return res;
}
VTK_ABI_NAMESPACE_END
