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
#ifndef viskores_cont_ColorTableMap_h
#define viskores_cont_ColorTableMap_h

#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/cont/ColorTableSamples.h>

#include <viskores/cont/Invoker.h>
#include <viskores/worklet/colorconversion/LookupTable.h>
#include <viskores/worklet/colorconversion/Portals.h>
#include <viskores/worklet/colorconversion/TransferFunction.h>

#include <viskores/exec/ColorTable.h>

namespace viskores
{
namespace cont
{

/// \brief Sample each value through an intermediate lookup/sample table to generate RGBA colors
///
/// Each value in \c values is binned based on its value in relationship to the range
/// of the color table and will use the color value at that bin from the \c samples.
/// To generate the lookup table use \c Sample .
///
/// Here is a simple example.
/// \code{.cpp}
///
/// viskores::cont::ColorTableSamplesRGBA samples;
/// viskores::cont::ColorTable table("black-body radiation");
/// table.Sample(256, samples);
/// viskores::cont::ArrayHandle<viskores::Vec4ui_8> colors;
/// viskores::cont::ColorTableMap(input, samples, colors);
///
/// \endcode
template <typename T, typename S>
bool ColorTableMap(const viskores::cont::ArrayHandle<T, S>& values,
                   const viskores::cont::ColorTableSamplesRGBA& samples,
                   viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut)
{
  if (samples.NumberOfSamples <= 0)
  {
    return false;
  }
  viskores::worklet::colorconversion::LookupTable lookupTable(samples);
  viskores::cont::Invoker invoke(viskores::cont::DeviceAdapterTagAny{});
  invoke(lookupTable, values, samples.Samples, rgbaOut);
  return true;
}

/// \brief Sample each value through an intermediate lookup/sample table to generate RGB colors
///
/// Each value in \c values is binned based on its value in relationship to the range
/// of the color table and will use the color value at that bin from the \c samples.
/// To generate the lookup table use \c Sample .
///
/// Here is a simple example.
/// \code{.cpp}
///
/// viskores::cont::ColorTableSamplesRGB samples;
/// viskores::cont::ColorTable table("black-body radiation");
/// table.Sample(256, samples);
/// viskores::cont::ArrayHandle<viskores::Vec3ui_8> colors;
/// viskores::cont::ColorTableMap(input, samples, colors);
///
/// \endcode
template <typename T, typename S>
bool ColorTableMap(const viskores::cont::ArrayHandle<T, S>& values,
                   const viskores::cont::ColorTableSamplesRGB& samples,
                   viskores::cont::ArrayHandle<viskores::Vec3ui_8>& rgbOut)
{
  if (samples.NumberOfSamples <= 0)
  {
    return false;
  }
  viskores::worklet::colorconversion::LookupTable lookupTable(samples);
  viskores::cont::Invoker invoke(viskores::cont::DeviceAdapterTagAny{});
  invoke(lookupTable, values, samples.Samples, rgbOut);
  return true;
}

/// \brief Use magnitude of a vector with a sample table to generate RGBA colors
///
template <typename T, int N, typename S>
bool ColorTableMapMagnitude(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                            const viskores::cont::ColorTableSamplesRGBA& samples,
                            viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut)
{
  using namespace viskores::worklet::colorconversion;
  return viskores::cont::ColorTableMap(
    viskores::cont::make_ArrayHandleTransform(values, MagnitudePortal()), samples, rgbaOut);
}

/// \brief Use magnitude of a vector with a sample table to generate RGB colors
///
template <typename T, int N, typename S>
bool ColorTableMapMagnitude(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                            const viskores::cont::ColorTableSamplesRGB& samples,
                            viskores::cont::ArrayHandle<viskores::Vec3ui_8>& rgbOut)
{
  using namespace viskores::worklet::colorconversion;
  return viskores::cont::ColorTableMap(
    viskores::cont::make_ArrayHandleTransform(values, MagnitudePortal()), samples, rgbOut);
}

/// \brief Use a single component of a vector with a sample table to generate RGBA colors
///
template <typename T, int N, typename S>
bool ColorTableMapComponent(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                            viskores::IdComponent comp,
                            const viskores::cont::ColorTableSamplesRGBA& samples,
                            viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut)
{
  using namespace viskores::worklet::colorconversion;
  return viskores::cont::ColorTableMap(
    viskores::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), samples, rgbaOut);
}

/// \brief Use a single component of a vector with a sample table to generate RGB colors
///
template <typename T, int N, typename S>
bool ColorTableMapComponent(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                            viskores::IdComponent comp,
                            const viskores::cont::ColorTableSamplesRGB& samples,
                            viskores::cont::ArrayHandle<viskores::Vec3ui_8>& rgbOut)
{
  using namespace viskores::worklet::colorconversion;
  return viskores::cont::ColorTableMap(
    viskores::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), samples, rgbOut);
}

/// \brief Interpolate each value through the color table to generate RGBA colors
///
/// Each value in \c values will be sampled through the entire color table
/// to determine a color.
///
/// Note: This is more costly than using Sample/Map with the generated intermediate lookup table
template <typename T, typename S>
bool ColorTableMap(const viskores::cont::ArrayHandle<T, S>& values,
                   const viskores::cont::ColorTable& table,
                   viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut)
{
  viskores::cont::Invoker invoke;
  invoke(viskores::worklet::colorconversion::TransferFunction{}, values, table, rgbaOut);
  return true;
}

/// \brief Interpolate each value through the color table to generate RGB colors
///
/// Each value in \c values will be sampled through the entire color table
/// to determine a color.
///
/// Note: This is more costly than using Sample/Map with the generated intermediate lookup table
template <typename T, typename S>
bool ColorTableMap(const viskores::cont::ArrayHandle<T, S>& values,
                   const viskores::cont::ColorTable& table,
                   viskores::cont::ArrayHandle<viskores::Vec<viskores::UInt8, 3>>& rgbOut)
{
  viskores::cont::Invoker invoke;
  invoke(viskores::worklet::colorconversion::TransferFunction{}, values, table, rgbOut);
  return true;
}

/// \brief Use magnitude of a vector to generate RGBA colors
///
template <typename T, int N, typename S>
bool ColorTableMapMagnitude(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                            const viskores::cont::ColorTable& table,
                            viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut)
{
  using namespace viskores::worklet::colorconversion;
  return viskores::cont::ColorTableMap(
    viskores::cont::make_ArrayHandleTransform(values, MagnitudePortal()), table, rgbaOut);
}

/// \brief Use magnitude of a vector to generate RGB colors
///
template <typename T, int N, typename S>
bool ColorTableMapMagnitude(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                            const viskores::cont::ColorTable& table,
                            viskores::cont::ArrayHandle<viskores::Vec<viskores::UInt8, 3>>& rgbOut)
{
  using namespace viskores::worklet::colorconversion;
  return viskores::cont::ColorTableMap(
    viskores::cont::make_ArrayHandleTransform(values, MagnitudePortal()), table, rgbOut);
}

/// \brief Use a single component of a vector to generate RGBA colors
///
template <typename T, int N, typename S>
bool ColorTableMapComponent(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                            viskores::IdComponent comp,
                            const viskores::cont::ColorTable& table,
                            viskores::cont::ArrayHandle<viskores::Vec4ui_8>& rgbaOut)
{
  using namespace viskores::worklet::colorconversion;
  return viskores::cont::ColorTableMap(
    viskores::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), table, rgbaOut);
}

/// \brief Use a single component of a vector to generate RGB colors
///
template <typename T, int N, typename S>
bool ColorTableMapComponent(const viskores::cont::ArrayHandle<viskores::Vec<T, N>, S>& values,
                            viskores::IdComponent comp,
                            const viskores::cont::ColorTable& table,
                            viskores::cont::ArrayHandle<viskores::Vec<viskores::UInt8, 3>>& rgbOut)
{
  using namespace viskores::worklet::colorconversion;
  return viskores::cont::ColorTableMap(
    viskores::cont::make_ArrayHandleTransform(values, ComponentPortal(comp)), table, rgbOut);
}
}
}
#endif // viskores_cont_ColorTableMap_h
