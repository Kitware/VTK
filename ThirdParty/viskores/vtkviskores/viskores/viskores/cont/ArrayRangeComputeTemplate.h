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
#ifndef viskores_cont_ArrayRangeComputeTemplate_h
#define viskores_cont_ArrayRangeComputeTemplate_h

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleDecorator.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/internal/ArrayRangeComputeUtils.h>

#include <viskores/BinaryOperators.h>
#include <viskores/Deprecated.h>
#include <viskores/VecFlat.h>
#include <viskores/VecTraits.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/internal/Instantiations.h>

namespace viskores
{
namespace cont
{

namespace internal
{

//-------------------------------------------------------------------------------------------------
struct ComputeRangeOptionsDecorator
{
  bool IgnoreInf = false;

  template <typename SrcPortal, typename MaskPortal>
  struct Functor
  {
    SrcPortal Src;
    MaskPortal Mask;
    bool IgnoreInf;

    using InValueType = typename SrcPortal::ValueType;
    using InVecTraits = viskores::VecTraits<InValueType>;
    using ResultType =
      viskores::Vec<viskores::Vec<viskores::Float64, InVecTraits::NUM_COMPONENTS>, 2>;

    VISKORES_EXEC_CONT
    ResultType operator()(viskores::Id idx) const
    {
      if ((this->Mask.GetNumberOfValues() != 0) && (this->Mask.Get(idx) == 0))
      {
        return { { viskores::Range{}.Min }, { viskores::Range{}.Max } };
      }

      const auto& inVal = this->Src.Get(idx);
      ResultType outVal;
      for (viskores::IdComponent i = 0; i < InVecTraits::NUM_COMPONENTS; ++i)
      {
        auto val = static_cast<viskores::Float64>(InVecTraits::GetComponent(inVal, i));
        if (viskores::IsNan(val) || (this->IgnoreInf && !viskores::IsFinite(val)))
        {
          outVal[0][i] = viskores::Range{}.Min;
          outVal[1][i] = viskores::Range{}.Max;
        }
        else
        {
          outVal[0][i] = outVal[1][i] = val;
        }
      }

      return outVal;
    }
  };

  template <typename SrcPortal, typename GhostPortal>
  Functor<SrcPortal, GhostPortal> CreateFunctor(const SrcPortal& sp, const GhostPortal& gp) const
  {
    return { sp, gp, this->IgnoreInf };
  }
};

template <typename ArrayHandleType>
struct ArrayValueIsNested
{
  static constexpr bool Value =
    !viskores::internal::IsFlatVec<typename ArrayHandleType::ValueType>::value;
};

template <typename ArrayHandleType, bool IsNested = ArrayValueIsNested<ArrayHandleType>::Value>
struct NestedToFlat;

template <typename ArrayHandleType>
struct NestedToFlat<ArrayHandleType, true>
{
  static auto Transform(const ArrayHandleType& in)
  {
    return viskores::cont::ArrayHandleCast<viskores::VecFlat<typename ArrayHandleType::ValueType>,
                                           ArrayHandleType>(in);
  }
};

template <typename ArrayHandleType>
struct NestedToFlat<ArrayHandleType, false>
{
  static auto Transform(const ArrayHandleType& in) { return in; }
};

template <typename ArrayHandleType>
inline auto NestedToFlatTransform(const ArrayHandleType& input)
{
  return NestedToFlat<ArrayHandleType>::Transform(input);
}

//-------------------------------------------------------------------------------------------------
/// \brief A generic implementation of `ArrayRangeCompute`. This is the implementation used
/// when `ArrayRangeComputeImpl` is not specialized.
///
template <typename T, typename S>
inline viskores::cont::ArrayHandle<viskores::Range> ArrayRangeComputeGeneric(
  const viskores::cont::ArrayHandle<T, S>& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "ArrayRangeCompute");

  using VecTraits = viskores::VecTraits<T>;

  viskores::cont::ArrayHandle<viskores::Range> range;
  range.Allocate(VecTraits::NUM_COMPONENTS);

  //We want to minimize the amount of code that we do in try execute as
  //it is repeated for each
  if (input.GetNumberOfValues() < 1)
  {
    range.Fill(viskores::Range{});
  }
  else
  {
    // if input is an array of nested vectors, transform them to `VecFlat` using ArrayHandleCast
    auto flattened = NestedToFlatTransform(input);
    ComputeRangeOptionsDecorator decorator{ computeFiniteRange };
    auto decorated =
      make_ArrayHandleDecorator(flattened.GetNumberOfValues(), decorator, flattened, maskArray);

    using ResultType =
      viskores::Vec<viskores::Vec<viskores::Float64, VecTraits::NUM_COMPONENTS>, 2>;
    using MinAndMaxFunctor = viskores::MinAndMax<typename ResultType::ComponentType>;
    ResultType identity{ { viskores::Range{}.Min }, { viskores::Range{}.Max } };

    auto result =
      viskores::cont::Algorithm::Reduce(device, decorated, identity, MinAndMaxFunctor{});

    auto portal = range.WritePortal();
    for (viskores::IdComponent i = 0; i < VecTraits::NUM_COMPONENTS; ++i)
    {
      portal.Set(i, viskores::Range(result[0][i], result[1][i]));
    }
  }

  return range;
}

//-------------------------------------------------------------------------------------------------
struct ScalarMagnitudeFunctor
{
  template <typename T>
  VISKORES_EXEC_CONT viskores::Float64 operator()(const T& val) const
  {
    // spcilization of `viskores::Magnitude` for scalars should avoid `sqrt` computation by using `abs`
    // instead
    return static_cast<viskores::Float64>(viskores::Magnitude(val));
  }
};

struct MagnitudeSquareFunctor
{
  template <typename T>
  VISKORES_EXEC_CONT viskores::Float64 operator()(const T& val) const
  {
    using VecTraits = viskores::VecTraits<T>;
    viskores::Float64 result = 0;
    for (viskores::IdComponent i = 0; i < VecTraits::GetNumberOfComponents(val); ++i)
    {
      auto comp = static_cast<viskores::Float64>(VecTraits::GetComponent(val, i));
      result += comp * comp;
    }
    return result;
  }
};

template <typename ArrayHandleType>
viskores::Range ArrayRangeComputeMagnitudeGenericImpl(
  viskores::VecTraitsTagSingleComponent,
  const ArrayHandleType& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device)
{
  auto mag = viskores::cont::make_ArrayHandleTransform(input, ScalarMagnitudeFunctor{});
  auto rangeAH = ArrayRangeComputeGeneric(mag, maskArray, computeFiniteRange, device);
  return rangeAH.ReadPortal().Get(0);
}

template <typename ArrayHandleType>
viskores::Range ArrayRangeComputeMagnitudeGenericImpl(
  viskores::VecTraitsTagMultipleComponents,
  const ArrayHandleType& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device)
{
  auto magsqr = viskores::cont::make_ArrayHandleTransform(input, MagnitudeSquareFunctor{});
  auto rangeAH = ArrayRangeComputeGeneric(magsqr, maskArray, computeFiniteRange, device);
  auto range = rangeAH.ReadPortal().Get(0);
  if (range.IsNonEmpty())
  {
    range.Min = viskores::Sqrt(range.Min);
    range.Max = viskores::Sqrt(range.Max);
  }
  return range;
}

/// \brief A generic implementation of `ArrayRangeComputeMagnitude`. This is the implementation used
/// when `ArrayRangeComputeMagnitudeImpl` is not specialized.
///
template <typename T, typename S>
inline viskores::Range ArrayRangeComputeMagnitudeGeneric(
  const viskores::cont::ArrayHandle<T, S>& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device)
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "ArrayRangeComputeMagnitude");

  using VecTraits = viskores::VecTraits<T>;

  //We want to minimize the amount of code that we do in try execute as
  //it is repeated for each
  if (input.GetNumberOfValues() < 1)
  {
    return viskores::Range{};
  }

  auto flattened = NestedToFlatTransform(input);
  return ArrayRangeComputeMagnitudeGenericImpl(
    typename VecTraits::HasMultipleComponents{}, flattened, maskArray, computeFiniteRange, device);
}

//-------------------------------------------------------------------------------------------------
template <typename S>
struct ArrayRangeComputeImpl
{
  template <typename T>
  viskores::cont::ArrayHandle<viskores::Range> operator()(
    const viskores::cont::ArrayHandle<T, S>& input,
    const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
    bool computeFiniteRange,
    viskores::cont::DeviceAdapterId device) const
  {
    return viskores::cont::internal::ArrayRangeComputeGeneric(
      input, maskArray, computeFiniteRange, device);
  }
};

template <typename S>
struct ArrayRangeComputeMagnitudeImpl
{
  template <typename T>
  viskores::Range operator()(const viskores::cont::ArrayHandle<T, S>& input,
                             const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
                             bool computeFiniteRange,
                             viskores::cont::DeviceAdapterId device) const
  {
    return viskores::cont::internal::ArrayRangeComputeMagnitudeGeneric(
      input, maskArray, computeFiniteRange, device);
  }
};

} // namespace internal

//-------------------------------------------------------------------------------------------------
/// @{
/// \brief Templated version of ArrayRangeCompute
/// \sa ArrayRangeCompute
///
template <typename T, typename S>
inline viskores::cont::ArrayHandle<viskores::Range> ArrayRangeComputeTemplate(
  const viskores::cont::ArrayHandle<T, S>& input,
  bool computeFiniteRange = false,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny{})
{
  return ArrayRangeComputeTemplate(
    input, viskores::cont::ArrayHandle<viskores::UInt8>{}, computeFiniteRange, device);
}

template <typename T, typename S>
viskores::cont::ArrayHandle<viskores::Range> ArrayRangeComputeTemplate(
  const viskores::cont::ArrayHandle<T, S>& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange = false,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny{})
{
  VISKORES_ASSERT(maskArray.GetNumberOfValues() == 0 ||
                  maskArray.GetNumberOfValues() == input.GetNumberOfValues());
  return internal::ArrayRangeComputeImpl<S>{}(input, maskArray, computeFiniteRange, device);
}

template <typename T, typename S>
inline viskores::cont::ArrayHandle<viskores::Range> ArrayRangeComputeTemplate(
  const viskores::cont::ArrayHandle<T, S>& input,
  viskores::cont::DeviceAdapterId device)
{
  return ArrayRangeComputeTemplate(input, false, device);
}

/// @}

/// @{
/// \brief Templated version of ArrayRangeComputeMagnitude
/// \sa ArrayRangeComputeMagnitude
///
template <typename T, typename S>
inline viskores::Range ArrayRangeComputeMagnitudeTemplate(
  const viskores::cont::ArrayHandle<T, S>& input,
  bool computeFiniteRange = false,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny{})
{
  return ArrayRangeComputeMagnitudeTemplate(
    input, viskores::cont::ArrayHandle<viskores::UInt8>{}, computeFiniteRange, device);
}

template <typename T, typename S>
viskores::Range ArrayRangeComputeMagnitudeTemplate(
  const viskores::cont::ArrayHandle<T, S>& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange = false,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny{})
{
  VISKORES_ASSERT(maskArray.GetNumberOfValues() == 0 ||
                  maskArray.GetNumberOfValues() == input.GetNumberOfValues());
  return internal::ArrayRangeComputeMagnitudeImpl<S>{}(
    input, maskArray, computeFiniteRange, device);
}

template <typename T, typename S>
inline viskores::Range ArrayRangeComputeMagnitudeTemplate(
  const viskores::cont::ArrayHandle<T, S>& input,
  viskores::cont::DeviceAdapterId device)
{
  return ArrayRangeComputeMagnitudeTemplate(input, false, device);
}
/// @}

//-----------------------------------------------------------------------------
template <typename ArrayHandleType>
VISKORES_DEPRECATED(2.1, "Use precompiled ArrayRangeCompute or ArrayRangeComputeTemplate.")
inline viskores::cont::ArrayHandle<viskores::Range> ArrayRangeCompute(
  const ArrayHandleType& input,
  viskores::cont::DeviceAdapterId device = viskores::cont::DeviceAdapterTagAny{})
{
  return ArrayRangeComputeTemplate(input, false, device);
}

}
} // namespace viskores::cont

#define VISKORES_ARRAY_RANGE_COMPUTE_DCLR(...)                                            \
  viskores::cont::ArrayHandle<viskores::Range> viskores::cont::ArrayRangeComputeTemplate( \
    const viskores::cont::ArrayHandle<__VA_ARGS__>&,                                      \
    const viskores::cont::ArrayHandle<viskores::UInt8>&,                                  \
    bool,                                                                                 \
    viskores::cont::DeviceAdapterId)

#define VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(...)                    \
  viskores::Range viskores::cont::ArrayRangeComputeMagnitudeTemplate( \
    const viskores::cont::ArrayHandle<__VA_ARGS__>&,                  \
    const viskores::cont::ArrayHandle<viskores::UInt8>&,              \
    bool,                                                             \
    viskores::cont::DeviceAdapterId)

#define VISKORES_ARRAY_RANGE_COMPUTE_INT_SCALARS(modifiers, ...)                  \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Int8, __VA_ARGS__);       \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Int8, __VA_ARGS__);   \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::UInt8, __VA_ARGS__);      \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::UInt8, __VA_ARGS__);  \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Int16, __VA_ARGS__);      \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Int16, __VA_ARGS__);  \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::UInt16, __VA_ARGS__);     \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::UInt16, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Int32, __VA_ARGS__);      \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Int32, __VA_ARGS__);  \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::UInt32, __VA_ARGS__);     \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::UInt32, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Int64, __VA_ARGS__);      \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Int64, __VA_ARGS__);  \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::UInt64, __VA_ARGS__);     \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::UInt64, __VA_ARGS__)

#define VISKORES_ARRAY_RANGE_COMPUTE_FLOAT_SCALARS(modifiers, ...)                 \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Float32, __VA_ARGS__);     \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Float32, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Float64, __VA_ARGS__);     \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Float64, __VA_ARGS__)

#define VISKORES_ARRAY_RANGE_COMPUTE_BOOL_SCALARS(modifiers, ...) \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(bool, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(bool, __VA_ARGS__)

#define VISKORES_ARRAY_RANGE_COMPUTE_OTHER_SCALARS(modifiers, ...)                               \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(char, __VA_ARGS__);                                \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(char, __VA_ARGS__);                            \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(signed VISKORES_UNUSED_INT_TYPE, __VA_ARGS__);     \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(signed VISKORES_UNUSED_INT_TYPE, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(unsigned VISKORES_UNUSED_INT_TYPE, __VA_ARGS__);   \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(unsigned VISKORES_UNUSED_INT_TYPE, __VA_ARGS__)

#define VISKORES_ARRAY_RANGE_COMPUTE_ALL_SCALARS(modifiers, ...)      \
  VISKORES_ARRAY_RANGE_COMPUTE_INT_SCALARS(modifiers, __VA_ARGS__);   \
  VISKORES_ARRAY_RANGE_COMPUTE_FLOAT_SCALARS(modifiers, __VA_ARGS__); \
  VISKORES_ARRAY_RANGE_COMPUTE_BOOL_SCALARS(modifiers, __VA_ARGS__);  \
  VISKORES_ARRAY_RANGE_COMPUTE_OTHER_SCALARS(modifiers, __VA_ARGS__)

#define VISKORES_ARRAY_RANGE_COMPUTE_INT_VECN(modifiers, N, ...)                                   \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<viskores::Int8, N>, __VA_ARGS__);      \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<viskores::Int8, N>, __VA_ARGS__);  \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<viskores::UInt8, N>, __VA_ARGS__);     \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<viskores::UInt8, N>, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<viskores::Int16, N>, __VA_ARGS__);     \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<viskores::Int16, N>, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<viskores::UInt16, N>, __VA_ARGS__);    \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<viskores::UInt16, N>,              \
                                                  __VA_ARGS__);                                    \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<viskores::Int32, N>, __VA_ARGS__);     \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<viskores::Int32, N>, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<viskores::UInt32, N>, __VA_ARGS__);    \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<viskores::UInt32, N>,              \
                                                  __VA_ARGS__);                                    \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<viskores::Int64, N>, __VA_ARGS__);     \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<viskores::Int64, N>, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<viskores::UInt64, N>, __VA_ARGS__);    \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<viskores::UInt64, N>, __VA_ARGS__)

#define VISKORES_ARRAY_RANGE_COMPUTE_FLOAT_VECN(modifiers, N, ...)                               \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<viskores::Float32, N>, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<viskores::Float32, N>,           \
                                                  __VA_ARGS__);                                  \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<viskores::Float64, N>, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<viskores::Float64, N>, __VA_ARGS__)

#define VISKORES_ARRAY_RANGE_COMPUTE_BOOL_VECN(modifiers, N, ...)                   \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<bool, N>, __VA_ARGS__); \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<bool, N>, __VA_ARGS__)

#define VISKORES_ARRAY_RANGE_COMPUTE_OTHER_VECN(modifiers, N, ...)                                 \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<char, N>, __VA_ARGS__);                \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(viskores::Vec<char, N>, __VA_ARGS__);            \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<signed VISKORES_UNUSED_INT_TYPE, N>,   \
                                              __VA_ARGS__);                                        \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(                                                 \
    viskores::Vec<signed VISKORES_UNUSED_INT_TYPE, N>, __VA_ARGS__);                               \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_DCLR(viskores::Vec<unsigned VISKORES_UNUSED_INT_TYPE, N>, \
                                              __VA_ARGS__);                                        \
  modifiers VISKORES_ARRAY_RANGE_COMPUTE_MAG_DCLR(                                                 \
    viskores::Vec<unsigned VISKORES_UNUSED_INT_TYPE, N>, __VA_ARGS__)

#define VISKORES_ARRAY_RANGE_COMPUTE_ALL_VECN(modifiers, N, ...)      \
  VISKORES_ARRAY_RANGE_COMPUTE_INT_VECN(modifiers, N, __VA_ARGS__);   \
  VISKORES_ARRAY_RANGE_COMPUTE_FLOAT_VECN(modifiers, N, __VA_ARGS__); \
  VISKORES_ARRAY_RANGE_COMPUTE_BOOL_VECN(modifiers, N, __VA_ARGS__);  \
  VISKORES_ARRAY_RANGE_COMPUTE_OTHER_VECN(modifiers, N, __VA_ARGS__)

namespace viskores
{
namespace cont
{

struct StorageTagSOA;

template <typename ST1, typename ST2, typename ST3>
struct StorageTagCartesianProduct;

struct StorageTagConstant;

struct StorageTagCounting;

struct StorageTagXGCCoordinates;

struct StorageTagStride;

}
} // viskores::cont

//-------------------------------------------------------------------------------------------------
/// @cond NOPE
VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_SCALARS(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                         viskores::cont::StorageTagBasic);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      2,
                                      viskores::cont::StorageTagBasic);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      3,
                                      viskores::cont::StorageTagBasic);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      4,
                                      viskores::cont::StorageTagBasic);
VISKORES_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      2,
                                      viskores::cont::StorageTagSOA);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      3,
                                      viskores::cont::StorageTagSOA);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      4,
                                      viskores::cont::StorageTagSOA);
VISKORES_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_FLOAT_VECN(
  extern template VISKORES_CONT_TEMPLATE_EXPORT,
  3,
  viskores::cont::StorageTagCartesianProduct<viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic,
                                             viskores::cont::StorageTagBasic>);
VISKORES_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_FLOAT_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                        3,
                                        StorageTagXGCCoordinates);
VISKORES_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_SCALARS(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                         viskores::cont::StorageTagConstant);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      2,
                                      viskores::cont::StorageTagConstant);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      3,
                                      viskores::cont::StorageTagConstant);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      4,
                                      viskores::cont::StorageTagConstant);
VISKORES_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_INT_SCALARS(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                         viskores::cont::StorageTagCounting);
VISKORES_ARRAY_RANGE_COMPUTE_FLOAT_SCALARS(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                           viskores::cont::StorageTagCounting);
VISKORES_ARRAY_RANGE_COMPUTE_OTHER_SCALARS(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                           viskores::cont::StorageTagCounting);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_INT_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      2,
                                      viskores::cont::StorageTagCounting);
VISKORES_ARRAY_RANGE_COMPUTE_FLOAT_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                        2,
                                        viskores::cont::StorageTagCounting);
VISKORES_ARRAY_RANGE_COMPUTE_OTHER_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                        2,
                                        viskores::cont::StorageTagCounting);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_INT_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      3,
                                      viskores::cont::StorageTagCounting);
VISKORES_ARRAY_RANGE_COMPUTE_FLOAT_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                        3,
                                        viskores::cont::StorageTagCounting);
VISKORES_ARRAY_RANGE_COMPUTE_OTHER_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                        3,
                                        viskores::cont::StorageTagCounting);
VISKORES_INSTANTIATION_END

VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_INT_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                      4,
                                      viskores::cont::StorageTagCounting);
VISKORES_ARRAY_RANGE_COMPUTE_FLOAT_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                        4,
                                        viskores::cont::StorageTagCounting);
VISKORES_ARRAY_RANGE_COMPUTE_OTHER_VECN(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                        4,
                                        viskores::cont::StorageTagCounting);
VISKORES_INSTANTIATION_END

//-------------------------------------------------------------------------------------------------
VISKORES_INSTANTIATION_BEGIN
VISKORES_ARRAY_RANGE_COMPUTE_ALL_SCALARS(extern template VISKORES_CONT_TEMPLATE_EXPORT,
                                         viskores::cont::StorageTagStride);
VISKORES_INSTANTIATION_END
/// @endcond

#endif //viskores_cont_ArrayRangeComputeTemplate_h
