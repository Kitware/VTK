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
#ifndef viskores_rendering_raytracing_PartialComposite_h
#define viskores_rendering_raytracing_PartialComposite_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/rendering/raytracing/ChannelBuffer.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

template <typename FloatType>
struct PartialComposite
{
  viskores::cont::ArrayHandle<viskores::Id> PixelIds; // pixel that owns composite
  viskores::cont::ArrayHandle<FloatType> Distances;   // distance of composite end
  ChannelBuffer<FloatType> Buffer;                    // holds either color or absorption
  // (optional fields)
  ChannelBuffer<FloatType> Intensities;               // holds the intensity emerging from each ray
  viskores::cont::ArrayHandle<FloatType> PathLengths; // Total distance traversed through the mesh
};
}
}
} // namespace viskores::rendering::raytracing
#endif
