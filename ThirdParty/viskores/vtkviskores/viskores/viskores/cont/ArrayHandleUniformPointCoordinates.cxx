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

#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>

#include <viskores/cont/ArrayRangeComputeTemplate.h>

#include <viskores/VectorAnalysis.h>

namespace viskores
{
namespace cont
{

ArrayHandleUniformPointCoordinates::ArrayHandleUniformPointCoordinates(viskores::Id3 dimensions,
                                                                       ValueType origin,
                                                                       ValueType spacing)
  : Superclass(internal::PortalToArrayHandleImplicitBuffers(
      viskores::internal::ArrayPortalUniformPointCoordinates(dimensions, origin, spacing)))
{
}

ArrayHandleUniformPointCoordinates::~ArrayHandleUniformPointCoordinates() = default;

viskores::Id3 ArrayHandleUniformPointCoordinates::GetDimensions() const
{
  return this->ReadPortal().GetDimensions();
}

viskores::Vec3f ArrayHandleUniformPointCoordinates::GetOrigin() const
{
  return this->ReadPortal().GetOrigin();
}

viskores::Vec3f ArrayHandleUniformPointCoordinates::GetSpacing() const
{
  return this->ReadPortal().GetSpacing();
}

namespace internal
{

viskores::cont::ArrayHandleStride<viskores::FloatDefault>
ArrayExtractComponentImpl<viskores::cont::StorageTagUniformPoints>::operator()(
  const viskores::cont::ArrayHandleUniformPointCoordinates& src,
  viskores::IdComponent componentIndex,
  viskores::CopyFlag allowCopy) const
{
  if (allowCopy != viskores::CopyFlag::On)
  {
    throw viskores::cont::ErrorBadValue(
      "Cannot extract component of ArrayHandleUniformPointCoordinates without copying. "
      "(However, the whole array does not need to be copied.)");
  }

  viskores::Id3 dims = src.GetDimensions();
  viskores::Vec3f origin = src.GetOrigin();
  viskores::Vec3f spacing = src.GetSpacing();

  // A "slow" way to create the data, but the array is probably short. It would probably take
  // longer to schedule something on a device. (Can change that later if use cases change.)
  viskores::cont::ArrayHandleBasic<viskores::FloatDefault> componentArray;
  componentArray.Allocate(dims[componentIndex]);
  auto portal = componentArray.WritePortal();
  for (viskores::Id i = 0; i < dims[componentIndex]; ++i)
  {
    portal.Set(i, origin[componentIndex] + (i * spacing[componentIndex]));
  }

  switch (componentIndex)
  {
    case 0:
      return viskores::cont::ArrayHandleStride<viskores::FloatDefault>(
        componentArray, src.GetNumberOfValues(), 1, 0, dims[0], 1);
    case 1:
      return viskores::cont::ArrayHandleStride<viskores::FloatDefault>(
        componentArray, src.GetNumberOfValues(), 1, 0, dims[1], dims[0]);
    case 2:
      return viskores::cont::ArrayHandleStride<viskores::FloatDefault>(
        componentArray, src.GetNumberOfValues(), 1, 0, 0, dims[0] * dims[1]);
    default:
      throw viskores::cont::ErrorBadValue("Bad index given to ArrayExtractComponent.");
  }
}

VISKORES_CONT viskores::cont::ArrayHandle<viskores::Range>
ArrayRangeComputeImpl<viskores::cont::StorageTagUniformPoints>::operator()(
  const viskores::cont::ArrayHandleUniformPointCoordinates& input,
  const viskores::cont::ArrayHandle<viskores::UInt8>& maskArray,
  bool computeFiniteRange,
  viskores::cont::DeviceAdapterId device) const
{
  if (maskArray.GetNumberOfValues() != 0)
  {
    return viskores::cont::internal::ArrayRangeComputeGeneric(
      input, maskArray, computeFiniteRange, device);
  }

  viskores::internal::ArrayPortalUniformPointCoordinates portal = input.ReadPortal();

  // In this portal we know that the min value is the first entry and the
  // max value is the last entry.
  viskores::Vec3f minimum = portal.Get(0);
  viskores::Vec3f maximum = portal.Get(portal.GetNumberOfValues() - 1);

  viskores::cont::ArrayHandle<viskores::Range> rangeArray;
  rangeArray.Allocate(3);
  viskores::cont::ArrayHandle<viskores::Range>::WritePortalType outPortal =
    rangeArray.WritePortal();
  outPortal.Set(0, viskores::Range(minimum[0], maximum[0]));
  outPortal.Set(1, viskores::Range(minimum[1], maximum[1]));
  outPortal.Set(2, viskores::Range(minimum[2], maximum[2]));

  return rangeArray;
}

} // namespace internal

}
} // namespace viskores::cont
