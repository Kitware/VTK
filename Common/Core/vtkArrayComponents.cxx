// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkArrayComponents.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkGenericDataArray.h"
#include "vtkSMPTools.h"
#include "vtkStringFormatter.h"
#include "vtkStringScanner.h"

namespace
{

std::string arrayName(vtkAbstractArray* array)
{
  std::string aname;
  if (array->GetName())
  {
    aname = array->GetName();
  }
  if (aname.empty())
  {
    aname = "unnamed";
  }
  return aname;
}

std::string componentName(vtkAbstractArray* array, int component)
{
  std::string cname;
  bool isIndex = false;
  switch (component)
  {
    case vtkArrayComponents::L1Norm:
      cname = "L₁";
      break;
    case vtkArrayComponents::L2Norm:
      cname = "L₂";
      break;
    case vtkArrayComponents::LInfNorm:
      cname = "L∞";
      break;
    default:
      isIndex = component >= 0;
      break;
  }

  if (isIndex)
  {
    if (array->HasAComponentName())
    {
      auto* compName = array->GetComponentName(component);
      if (compName && compName[0])
      {
        cname = compName;
      }
    }

    if (cname.empty())
    {
      cname = vtk::to_string(component);
    }
  }

  return cname;
}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4146) /* unary minus operator applied to unsigned type. */
#endif
template <typename Number, bool IsSigned = std::is_signed<Number>::value>
Number absolute(Number number)
{
  if (IsSigned)
  {
    return number < 0 ? -number : number;
  }
  return number;
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

struct InfNorm
{
  template <typename ArraySrc, typename ArrayDst>
  void operator()(ArraySrc* srcArray, ArrayDst* dstArray) const
  {
    using SrcType = vtk::GetAPIType<ArraySrc>;

    int numComp = srcArray->GetNumberOfComponents();

    const auto srcRange = vtk::DataArrayValueRange(srcArray);
    auto dstRange = vtk::DataArrayValueRange(dstArray);
    assert(srcRange.size() == numComp * dstRange.size());

    auto srcIter = srcRange.begin();
    auto dstIter = dstRange.begin();
    for (vtkIdType ii = 0; ii < static_cast<vtkIdType>(dstRange.size()); ++ii)
    {
      SrcType norm = absolute(static_cast<SrcType>(*srcIter++));
      for (int jj = 1; jj < numComp; ++jj)
      {
        auto val = absolute(static_cast<SrcType>(*srcIter++));
        if (val > norm)
        {
          norm = val;
        }
      }
      *dstIter = norm;
      ++dstIter;
    }
  }
};

vtkSmartPointer<vtkDataArray> computeL1Norm(vtkDataArray* array)
{
  auto norm = vtkSmartPointer<vtkDoubleArray>::New();
  norm->SetNumberOfTuples(array->GetNumberOfTuples());
  vtkSMPTools::For(0, array->GetNumberOfTuples(),
    [&norm, array](vtkIdType begin, vtkIdType end)
    {
      int numComponents = array->GetNumberOfComponents();
      std::vector<double> tuple(numComponents);
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        array->GetTuple(ii, tuple.data());
        double sum = 0;
        for (int jj = 0; jj < numComponents; ++jj)
        {
          sum += absolute(tuple[jj]);
        }
        norm->SetTuple1(ii, sum);
      }
    });
  return norm;
}

vtkSmartPointer<vtkDataArray> computeL2Norm(vtkDataArray* array)
{
  auto norm = vtkSmartPointer<vtkDoubleArray>::New();
  norm->SetNumberOfTuples(array->GetNumberOfTuples());
  vtkSMPTools::For(0, array->GetNumberOfTuples(),
    [&norm, array](vtkIdType begin, vtkIdType end)
    {
      int numComponents = array->GetNumberOfComponents();
      std::vector<double> tuple(numComponents);
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        array->GetTuple(ii, tuple.data());
        double norm2 = 0;
        for (int jj = 0; jj < numComponents; ++jj)
        {
          norm2 += tuple[jj] * tuple[jj];
        }
        norm->SetValue(ii, std::sqrt(norm2));
      }
    });
  return norm;
}

vtkSmartPointer<vtkDataArray> computeLInfNorm(vtkDataArray* array)
{
  vtkSmartPointer<vtkDataArray> norm;
  norm.TakeReference(array->NewInstance());
  norm->SetNumberOfTuples(array->GetNumberOfTuples());
  if (!vtkArrayDispatch::Dispatch2::Execute(array, norm, InfNorm{}))
  {
    vtkSMPTools::For(0, array->GetNumberOfTuples(),
      [&norm, array](vtkIdType begin, vtkIdType end)
      {
        int numComponents = array->GetNumberOfComponents();
        assert(numComponents > 1);
        std::vector<double> tuple(numComponents);
        for (vtkIdType ii = begin; ii < end; ++ii)
        {
          array->GetTuple(ii, tuple.data());
          double max = std::abs(tuple[0]);
          for (int jj = 1; jj < numComponents; ++jj)
          {
            max = std::max(max, std::abs(tuple[jj]));
          }
          norm->SetTuple1(ii, max);
        }
      });
  }
  return norm;
}

} // anonymous namespace

namespace vtk
{

VTK_ABI_NAMESPACE_BEGIN

int ArrayComponents(const std::string& enumerantStr)
{
  if (enumerantStr == "vtkArrayComponents::L1Norm" || enumerantStr == "L1Norm" ||
    enumerantStr == "L₁norm" || enumerantStr == "L₁ norm" || enumerantStr == "||·||₁")
  {
    return static_cast<int>(vtkArrayComponents::L1Norm);
  }
  else if (enumerantStr == "vtkArrayComponents::L2Norm" || enumerantStr == "L2Norm" ||
    enumerantStr == "L₂norm" || enumerantStr == "L₂ norm" || enumerantStr == "||·||₂")
  {
    return static_cast<int>(vtkArrayComponents::L2Norm);
  }
  else if (enumerantStr == "vtkArrayComponents::LInfNorm" || enumerantStr == "LInfNorm" ||
    enumerantStr == "L∞norm" || enumerantStr == "L∞ norm" || enumerantStr == "||·||∞")
  {
    return static_cast<int>(vtkArrayComponents::LInfNorm);
  }
  else if (enumerantStr == "vtkArrayComponents::AllComponents" || enumerantStr == "AllComponents" ||
    enumerantStr == "all components")
  {
    return static_cast<int>(vtkArrayComponents::AllComponents);
  }
  else if (enumerantStr == "vtkArrayComponents::Requested" || enumerantStr == "Requested" ||
    enumerantStr == "requested" || enumerantStr == "requested components")
  {
    return static_cast<int>(vtkArrayComponents::Requested);
  }
  // No enumerants match; assume it is an integer.
  return vtk::scan_int<int>(enumerantStr)->value();
}

std::string to_string(vtkArrayComponents enumerant)
{
  switch (enumerant)
  {
    case AllComponents:
      return "all components";
    case Requested:
      return "requested";
    case L1Norm:
      return "L₁ norm";
    case L2Norm:
      return "L₂ norm";
    case LInfNorm:
      return "L∞ norm";
    default:
      break;
  }
  return vtk::to_string(static_cast<int>(enumerant));
}

vtkSmartPointer<vtkAbstractArray> ComponentOrNormAsArray(vtkAbstractArray* array, int compOrNorm)
{
  vtkSmartPointer<vtkAbstractArray> result;
  if (!array)
  {
    return result;
  }
  if (compOrNorm == vtkArrayComponents::AllComponents ||
    (array->GetNumberOfComponents() == 1 && compOrNorm == 0))
  {
    // Return the input array; don't create a new one since it would be an identical copy.
    result = array;
  }
  else
  {
    if (auto dataArray = vtkDataArray::SafeDownCast(array))
    {
      switch (compOrNorm)
      {
        case vtkArrayComponents::L1Norm:
          result = computeL1Norm(dataArray);
          break;
        case vtkArrayComponents::L2Norm:
          result = computeL2Norm(dataArray);
          break;
        case vtkArrayComponents::LInfNorm:
          result = computeLInfNorm(dataArray);
          break;
        default:
        {
          if (compOrNorm >= 0 && compOrNorm < array->GetNumberOfComponents())
          {
            result.TakeReference(array->NewInstance());
            result->SetNumberOfComponents(1);
            result->SetNumberOfTuples(array->GetNumberOfTuples());
            result->CopyComponent(0, array, compOrNorm);
          }
          else
          {
            vtkErrorWithObjectMacro(array, "Invalid component " << compOrNorm << " requested.");
          }
        }
        break;
      }
      if (result)
      {
        std::string aname = arrayName(array);
        std::string cname = componentName(array, compOrNorm);
        result->SetName((aname + "_" + cname).c_str());
      }
    }
    else
    {
      // Variant and string arrays do not provide a norm nor allow an out-of-range component.
      if (compOrNorm < 0 || compOrNorm >= array->GetNumberOfComponents())
      {
        vtkErrorWithObjectMacro(array,
          "Request for an non-existent component or a norm on an array that does not support it.");
        return result;
      }

      // For variant and string arrays, we must create a new single-component array
      // and perform a copy as vtkImplicitArray does not work for those types yet.
      result.TakeReference(array->NewInstance());
      std::string aname = arrayName(array);
      std::string cname = componentName(array, compOrNorm);
      result->SetName((aname + "_" + cname).c_str());
      result->SetNumberOfComponents(1);
      result->SetNumberOfTuples(array->GetNumberOfTuples());
      result->CopyComponent(0, array, compOrNorm);
    }
  }
  return result;
}

VTK_ABI_NAMESPACE_END

} // namespace vtk
