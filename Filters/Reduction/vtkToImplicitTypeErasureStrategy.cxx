/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkToImplicitTypeErasureStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToImplicitTypeErasureStrategy.h"

#include "vtkArrayDispatch.h"
#include "vtkArrayDispatchImplicitArrayList.h"
#include "vtkDataArrayRange.h"
#include "vtkImplicitArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkToImplicitStrategy.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <type_traits>

namespace
{
const std::vector<int> BYTE_SIZES({ 1, 2, 4, 8 });
using Dispatch = vtkArrayDispatch::DispatchByArray<vtkArrayDispatch::AllArrays>;

template <typename ValueType, typename ArrayT>
struct TypeErasingBackend
{
public:
  TypeErasingBackend(ValueType minEl, ArrayT* arr)
    : Array(arr)
    , MinimumElement(minEl)
  {
  }

  ValueType operator()(int idx) const
  {
    return this->MinimumElement + static_cast<ValueType>(this->Array->GetValue(idx));
  }

private:
  vtkSmartPointer<ArrayT> Array;
  ValueType MinimumElement;
};

template <typename ValueType>
struct TypeErasingBackend<ValueType, vtkDataArray>
{
public:
  TypeErasingBackend(ValueType minEl, vtkDataArray* arr)
    : Array(arr)
    , Range(vtk::DataArrayValueRange(arr))
    , MinimumElement(minEl)
  {
  }

  ValueType operator()(int idx) const
  {
    return this->MinimumElement + static_cast<ValueType>(this->Range[idx]);
  }

private:
  vtkSmartPointer<vtkDataArray> Array;
  vtk::detail::ValueRange<vtkDataArray, vtk::detail::DynamicTupleSize> Range;
  ValueType MinimumElement;
};

struct ReductionChecker
{
  template <typename ArrayT>
  void operator()(ArrayT* arr, Option<double>& reduction)
  {
    using VType = vtk::GetAPIType<ArrayT>;
    if (!std::is_integral<VType>::value)
    {
      reduction = Option<double>();
      return;
    }
    auto range = vtk::DataArrayValueRange(arr);
    auto maxElem = std::max_element(range.begin(), range.end());
    auto minElem = std::min_element(range.begin(), range.end());
    VType span = *maxElem - *minElem;
    std::size_t nBytes = std::ceil(std::log2(span) / 8);
    reduction =
      (nBytes < sizeof(VType) ? Option<double>(static_cast<double>(nBytes) / sizeof(VType))
                              : Option<double>());
  }
};

struct TypeErasureReductor
{
  template <typename ArrayT, typename TargetArrayT>
  vtkSmartPointer<vtkDataArray> ConstructTypeErasedArray(vtk::GetAPIType<ArrayT> minEl, ArrayT* arr)
  {
    using BaseType = vtk::GetAPIType<ArrayT>;
    using TargetType = vtk::GetAPIType<TargetArrayT>;
    auto range = vtk::DataArrayValueRange(arr);
    vtkNew<TargetArrayT> diffArr;
    diffArr->SetNumberOfComponents(1);
    diffArr->SetNumberOfTuples(arr->GetNumberOfValues());
    auto diffRange = vtk::DataArrayValueRange(diffArr);
    std::transform(range.begin(), range.end(), diffRange.begin(),
      [&](BaseType val) { return static_cast<TargetType>(val - minEl); });
    vtkNew<vtkImplicitArray<TypeErasingBackend<BaseType, TargetArrayT>>> res;
    res->SetBackend(std::make_shared<TypeErasingBackend<BaseType, TargetArrayT>>(minEl, diffArr));
    res->SetNumberOfComponents(arr->GetNumberOfComponents());
    res->SetNumberOfTuples(arr->GetNumberOfTuples());
    res->SetName(arr->GetName());
    return res;
  }

  template <typename ArrayT>
  void operator()(ArrayT* arr, vtkSmartPointer<vtkDataArray>& result)
  {
    using VType = vtk::GetAPIType<ArrayT>;
    if (!std::is_integral<VType>::value)
    {
      result = nullptr;
      return;
    }
    auto range = vtk::DataArrayValueRange(arr);
    auto maxElem = std::max_element(range.begin(), range.end());
    auto minElem = std::min_element(range.begin(), range.end());
    VType span = *maxElem - *minElem;
    std::size_t nBytes = std::ceil(std::log2(span) / 8);
    auto itSize = std::upper_bound(BYTE_SIZES.begin(), BYTE_SIZES.end(), nBytes);
    switch (*itSize)
    {
      case 1:
        result = this->ConstructTypeErasedArray<ArrayT, vtkUnsignedCharArray>(*minElem, arr);
        return;
      case 2:
        result = this->ConstructTypeErasedArray<ArrayT, vtkUnsignedShortArray>(*minElem, arr);
        return;
      case 4:
        result = this->ConstructTypeErasedArray<ArrayT, vtkUnsignedLongArray>(*minElem, arr);
        return;
      case 8:
        result = this->ConstructTypeErasedArray<ArrayT, vtkUnsignedLongLongArray>(*minElem, arr);
        return;
      default:
        vtkWarningWithObjectMacro(nullptr, "Byte size received from processing is out of bounds");
        return;
    }
  }
};

}

VTK_ABI_NAMESPACE_BEGIN
//-------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkToImplicitTypeErasureStrategy);

//-------------------------------------------------------------------------
void vtkToImplicitTypeErasureStrategy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << std::flush;
}

//-------------------------------------------------------------------------
Option<double> vtkToImplicitTypeErasureStrategy::EstimateReduction(vtkDataArray* arr)
{
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to type erased array.");
    return Option<double>();
  }
  int nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return Option<double>();
  }
  Option<double> reduction;
  ::ReductionChecker checker;
  ::Dispatch::Execute(arr, checker, reduction);
  return reduction;
}

//-------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkToImplicitTypeErasureStrategy::Reduce(vtkDataArray* arr)
{
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to type erased array.");
    return nullptr;
  }
  int nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return nullptr;
  }
  vtkSmartPointer<vtkDataArray> res;
  ::TypeErasureReductor generator;
  ::Dispatch::Execute(arr, generator, res);
  return res;
}
VTK_ABI_NAMESPACE_END
