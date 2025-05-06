//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/StaticAssert.h>
#include <viskores/TypeTraits.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/filter/field_transform/Warp.h>
#include <viskores/worklet/WorkletMapField.h>

#include <type_traits>

namespace
{

struct WarpWorklet : viskores::worklet::WorkletMapField
{
  viskores::FloatDefault ScaleFactor;

  VISKORES_CONT explicit WarpWorklet(viskores::FloatDefault scale)
    : ScaleFactor(scale)
  {
  }

  using ControlSignature = void(FieldIn pointCoordinates,
                                FieldIn directions,
                                FieldIn scales,
                                FieldOut result);

  template <typename PointType, typename DirectionType, typename ScaleType, typename ResultType>
  VISKORES_EXEC void operator()(const PointType& point,
                                const DirectionType& direction,
                                ScaleType scale,
                                ResultType& result) const
  {
    viskores::IdComponent numComponents = result.GetNumberOfComponents();
    VISKORES_ASSERT(point.GetNumberOfComponents() == numComponents);
    VISKORES_ASSERT(direction.GetNumberOfComponents() == numComponents);

    result = direction;
    result *= scale * this->ScaleFactor;
    result += point;
  }
};

// The warp filter operates on 3 arrays: coordiantes, directions, and scale factors. Rather than
// try to satisfy every possible array type we expect, which would add to a lot of possibilities
// (especially because we add the constant varieties), we will just extract components as either
// `viskores::Float32` or `viskores::Float64`. That way for each we just need just 6 combinations. We can
// do this by extracting arrays by components using `UnknownArrayHandle`'s
// `ExtractArrayFromComponents`.
template <typename Functor>
VISKORES_CONT void CastAndCallExtractedArrayFloats(const viskores::cont::UnknownArrayHandle& array,
                                                   Functor&& functor)
{
  if (array.IsBaseComponentType<viskores::Float32>())
  {
    functor(array.ExtractArrayFromComponents<viskores::Float32>());
  }
  else if (array.IsBaseComponentType<viskores::Float64>())
  {
    functor(array.ExtractArrayFromComponents<viskores::Float64>());
  }
  else
  {
    // Array is not float. Copy it to a float array and call the functor.
    viskores::cont::ArrayHandleRuntimeVec<viskores::FloatDefault> arrayCopy{
      array.GetNumberOfComponentsFlat()
    };
    viskores::cont::ArrayCopy(array, arrayCopy);

    // We could call the functor directly on arrayCopy. But that would add a third
    // type of array. We would like to limit it to 2 types. Thus, stuff the known
    // array into its own `UnknownArrayHandle` and get an extracted array that will
    // match the others.
    viskores::cont::UnknownArrayHandle arrayCopyContainer = arrayCopy;
    functor(arrayCopyContainer.ExtractArrayFromComponents<viskores::FloatDefault>());
  }
}

template <typename T1, typename T2>
struct BiggerTypeImpl
{
  VISKORES_STATIC_ASSERT((std::is_same<typename viskores::TypeTraits<T1>::NumericTag,
                                       viskores::TypeTraitsRealTag>::value));
  VISKORES_STATIC_ASSERT((std::is_same<typename viskores::TypeTraits<T2>::NumericTag,
                                       viskores::TypeTraitsRealTag>::value));
  VISKORES_STATIC_ASSERT((std::is_same<typename viskores::TypeTraits<T1>::DimensionalityTag,
                                       viskores::TypeTraitsScalarTag>::value));
  VISKORES_STATIC_ASSERT((std::is_same<typename viskores::TypeTraits<T2>::DimensionalityTag,
                                       viskores::TypeTraitsScalarTag>::value));
  using type = std::conditional_t<(sizeof(T1) > sizeof(T2)), T1, T2>;
};
template <typename T1, typename T2>
using BiggerType = typename BiggerTypeImpl<T1, T2>::type;

template <typename CoordinateType, typename DirectionType, typename ScalarFactorType>
VISKORES_CONT viskores::cont::UnknownArrayHandle ComputeWarp(
  const viskores::cont::Invoker& invoke,
  const viskores::cont::ArrayHandleRecombineVec<CoordinateType>& points,
  const viskores::cont::ArrayHandleRecombineVec<DirectionType>& directions,
  const viskores::cont::ArrayHandleRecombineVec<ScalarFactorType>& scales,
  viskores::FloatDefault scaleFactor)
{
  viskores::IdComponent numComponents = points.GetNumberOfComponents();
  if (directions.GetNumberOfComponents() != numComponents)
  {
    throw viskores::cont::ErrorBadValue(
      "Number of components for points and directions does not agree.");
  }

  if (scales.GetNumberOfComponents() != 1)
  {
    throw viskores::cont::ErrorBadValue("ScaleField must be scalars, but they are not.");
  }
  auto scalarFactorsComponents = scales.GetComponentArray(0);

  using ResultType = BiggerType<BiggerType<CoordinateType, DirectionType>, ScalarFactorType>;
  viskores::cont::ArrayHandleRuntimeVec<ResultType> result{ numComponents };

  invoke(WarpWorklet{ scaleFactor }, points, directions, scalarFactorsComponents, result);

  return result;
}

template <typename CoordinateType, typename DirectionType>
VISKORES_CONT viskores::cont::UnknownArrayHandle ComputeWarp(
  const viskores::cont::Invoker& invoke,
  const viskores::cont::ArrayHandleRecombineVec<CoordinateType>& points,
  const viskores::cont::ArrayHandleRecombineVec<DirectionType>& directions,
  const viskores::cont::UnknownArrayHandle& scales,
  viskores::FloatDefault scaleFactor)
{
  viskores::cont::UnknownArrayHandle result;
  auto functor = [&](auto concrete)
  { result = ComputeWarp(invoke, points, directions, concrete, scaleFactor); };
  CastAndCallExtractedArrayFloats(scales, functor);
  return result;
}

template <typename CoordinateType>
VISKORES_CONT viskores::cont::UnknownArrayHandle ComputeWarp(
  const viskores::cont::Invoker& invoke,
  const viskores::cont::ArrayHandleRecombineVec<CoordinateType>& points,
  const viskores::cont::UnknownArrayHandle& directions,
  const viskores::cont::UnknownArrayHandle& scales,
  viskores::FloatDefault scaleFactor)
{
  viskores::cont::UnknownArrayHandle result;
  auto functor = [&](auto concrete)
  { result = ComputeWarp(invoke, points, concrete, scales, scaleFactor); };
  CastAndCallExtractedArrayFloats(directions, functor);
  return result;
}

VISKORES_CONT viskores::cont::UnknownArrayHandle ComputeWarp(
  const viskores::cont::Invoker& invoke,
  const viskores::cont::UnknownArrayHandle& points,
  const viskores::cont::UnknownArrayHandle& directions,
  const viskores::cont::UnknownArrayHandle& scales,
  viskores::FloatDefault scaleFactor)
{
  viskores::cont::UnknownArrayHandle result;
  auto functor = [&](auto concrete)
  { result = ComputeWarp(invoke, concrete, directions, scales, scaleFactor); };
  CastAndCallExtractedArrayFloats(points, functor);
  return result;
}

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace field_transform
{

//-----------------------------------------------------------------------------
VISKORES_CONT Warp::Warp()
{
  this->SetOutputFieldName("Warp");
  this->SetUseCoordinateSystemAsField(0, true);
  this->SetActiveField(1, "direction", viskores::cont::Field::Association::Points);
  this->SetActiveField(2, "scale", viskores::cont::Field::Association::Points);
}

//-----------------------------------------------------------------------------
VISKORES_CONT viskores::cont::DataSet Warp::DoExecute(const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::Field pointField = this->GetFieldFromDataSet(0, inDataSet);
  viskores::cont::UnknownArrayHandle points = pointField.GetData();

  viskores::cont::UnknownArrayHandle directions;
  if (this->GetUseConstantDirection())
  {
    directions = viskores::cont::make_ArrayHandleConstant(this->GetConstantDirection(),
                                                          points.GetNumberOfValues());
  }
  else
  {
    directions = this->GetFieldFromDataSet(1, inDataSet).GetData();
  }

  viskores::cont::UnknownArrayHandle scaleFactors;
  if (this->GetUseScaleField())
  {
    scaleFactors = this->GetFieldFromDataSet(2, inDataSet).GetData();
  }
  else
  {
    scaleFactors = viskores::cont::make_ArrayHandleConstant<viskores::FloatDefault>(
      1, points.GetNumberOfValues());
  }

  viskores::cont::UnknownArrayHandle warpedPoints =
    ComputeWarp(this->Invoke, points, directions, scaleFactors, this->ScaleFactor);

  if (this->GetChangeCoordinateSystem())
  {
    auto fieldMapper = [](viskores::cont::DataSet& out, const viskores::cont::Field& fieldToPass)
    { out.AddField(fieldToPass); };
    return this->CreateResultCoordinateSystem(
      inDataSet, inDataSet.GetCellSet(), this->GetOutputFieldName(), warpedPoints, fieldMapper);
  }
  else
  {
    return this->CreateResultField(
      inDataSet, this->GetOutputFieldName(), pointField.GetAssociation(), warpedPoints);
  }
}

} // namespace field_transform
} // namespace filter
} // namespace viskores
