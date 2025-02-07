// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkToImplicitTypeErasureStrategy.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkImplicitArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkToImplicitStrategy.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <type_traits>

namespace
{
const std::array<unsigned char, 4> BYTE_SIZES{ 1, 2, 4, 8 };
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

  ValueType operator()(vtkIdType idx) const
  {
    return this->MinimumElement + static_cast<ValueType>(this->Array->GetValue(idx));
  }

  unsigned long getMemorySize() const { return this->Array->GetActualMemorySize(); }

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

  ValueType operator()(vtkIdType idx) const
  {
    return this->MinimumElement + static_cast<ValueType>(this->Range[idx]);
  }

  unsigned long getMemorySize() const { return this->Array->GetActualMemorySize(); }

private:
  vtkSmartPointer<vtkDataArray> Array;
  vtk::detail::ValueRange<vtkDataArray, vtk::detail::DynamicTupleSize> Range;
  ValueType MinimumElement;
};

struct ReductionChecker
{
  template <typename ArrayT>
  void operator()(ArrayT* arr, vtkToImplicitStrategy::Optional& reduction)
  {
    using VType = vtk::GetAPIType<ArrayT>;
    if (!std::is_integral<VType>::value)
    {
      reduction = vtkToImplicitStrategy::Optional();
      return;
    }
    auto range = vtk::DataArrayValueRange(arr);
    auto pair = std::minmax_element(range.begin(), range.end());
    auto minElem = pair.first;
    auto maxElem = pair.second;
    VType span = *maxElem - *minElem;
    double nBytes = static_cast<double>(vtkMath::CeilLog2(span)) / 8.0;
    nBytes = *std::upper_bound(BYTE_SIZES.begin(), BYTE_SIZES.end(), nBytes);
    reduction = (nBytes < sizeof(VType) ? vtkToImplicitStrategy::Optional(nBytes / sizeof(VType))
                                        : vtkToImplicitStrategy::Optional());
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
    auto pair = std::minmax_element(range.begin(), range.end());
    auto minElem = pair.first;
    auto maxElem = pair.second;
    VType span = *maxElem - *minElem;
    double nBytes = static_cast<double>(vtkMath::CeilLog2(span)) / 8.0;
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
vtkToImplicitStrategy::Optional vtkToImplicitTypeErasureStrategy::EstimateReduction(
  vtkDataArray* arr)
{
  if (!arr)
  {
    vtkWarningMacro("Cannot transform nullptr to type erased array.");
    return vtkToImplicitStrategy::Optional();
  }
  vtkIdType nVals = arr->GetNumberOfValues();
  if (!nVals)
  {
    return vtkToImplicitStrategy::Optional();
  }
  vtkToImplicitStrategy::Optional reduction;
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
  vtkIdType nVals = arr->GetNumberOfValues();
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
