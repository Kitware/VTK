/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkToAffineArrayStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToAffineArrayStrategy.h"

#include "vtkAffineArray.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchImplicitArrayList.h"
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
  os << indent << "vtkToAffineArrayStrategy: \n";
  this->Superclass::PrintSelf(os, indent);
  os << std::flush;
}

//-------------------------------------------------------------------------
Option<double> vtkToAffineArrayStrategy::EstimateReduction(vtkDataArray* arr)
{
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to affine array.");
    return Option<double>();
  }
  int nVals = arr->GetNumberOfTuples() * arr->GetNumberOfComponents();
  if (!nVals)
  {
    return Option<double>();
  }
  ::AffineChecker checker;
  bool isAffine = false;
  if (!Dispatch::Execute(arr, checker, this->Tolerance, isAffine))
  {
    checker(arr, this->Tolerance, isAffine);
  }
  return isAffine ? Option<double>(2.0 / nVals) : Option<double>();
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
  int nVals = arr->GetNumberOfTuples() * arr->GetNumberOfComponents();
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
