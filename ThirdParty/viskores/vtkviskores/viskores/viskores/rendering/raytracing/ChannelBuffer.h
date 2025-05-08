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
#ifndef viskores_rendering_raytracing_ChannelBuffer_h
#define viskores_rendering_raytracing_ChannelBuffer_h

#include <viskores/cont/ArrayHandle.h>

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/rendering/raytracing/RayTracingTypeDefs.h>

#include <string>

namespace viskores
{
namespace rendering
{
namespace raytracing
{
///
///  \brief Mananges a buffer that contains many channels per value (e.g., RGBA values).
///
///  \c The ChannelBuffer class is meant to handle a buffer of values with potentially many
///  channels. While RGBA values could be placed in a Vec<T,4>, data with a large number of
///  channels (e.g., 100+ energy bins) are better handled by a raw array. Rays can have color,
///  absorption, absorption + emission, or even track additional scalar values to support
///  standards such as Cinema. This class allows us to treat all of these different use cases
///  with the same type.
///
///  This class has methods that can be utilized by other Viskores classes that already have a
///  a device dapter specified, and can be used by external callers where the call executes
///  on a device through the try execute mechanism.
///
///  \c Currently, the supported types are floating point to match the precision of the rays.
///

template <typename Precision>
class VISKORES_RENDERING_EXPORT ChannelBuffer
{
protected:
  viskores::Int32 NumChannels;
  viskores::Id Size;
  std::string Name;
  friend class ChannelBufferOperations;

public:
  viskores::cont::ArrayHandle<Precision> Buffer;

  /// Functions we want accessble outside of viskores some of which execute
  /// on a device
  ///
  VISKORES_CONT
  ChannelBuffer();

  VISKORES_CONT
  ChannelBuffer(const viskores::Int32 numChannels, const viskores::Id size);

  VISKORES_CONT
  ChannelBuffer<Precision> GetChannel(const viskores::Int32 channel);

  ChannelBuffer<Precision> ExpandBuffer(viskores::cont::ArrayHandle<viskores::Id> sparseIndexes,
                                        const viskores::Id outputSize,
                                        viskores::cont::ArrayHandle<Precision> signature);

  ChannelBuffer<Precision> ExpandBuffer(viskores::cont::ArrayHandle<viskores::Id> sparseIndexes,
                                        const viskores::Id outputSize,
                                        Precision initValue = 1.f);

  ChannelBuffer<Precision> Copy();

  void InitConst(const Precision value);
  void InitChannels(const viskores::cont::ArrayHandle<Precision>& signature);
  void Normalize(bool invert);
  void SetName(const std::string name);
  void Resize(const viskores::Id newSize);
  void SetNumChannels(const viskores::Int32 numChannels);
  viskores::Int32 GetNumChannels() const;
  viskores::Id GetSize() const;
  viskores::Id GetBufferLength() const;
  std::string GetName() const;
  void AddBuffer(const ChannelBuffer<Precision>& other);
  void MultiplyBuffer(const ChannelBuffer<Precision>& other);
  /// Functions that are calleble within viskores where the device is already known
  ///
  template <typename Device>
  VISKORES_CONT ChannelBuffer(const viskores::Int32 size, const viskores::Int32 numChannels, Device)
  {
    if (size < 1)
      throw viskores::cont::ErrorBadValue("ChannelBuffer: Size must be greater that 0");
    if (numChannels < 1)
      throw viskores::cont::ErrorBadValue("ChannelBuffer: NumChannels must be greater that 0");

    this->NumChannels = numChannels;
    this->Size = size;

    viskores::cont::Token token;
    this->Buffer.PrepareForOutput(this->Size * this->NumChannels, Device(), token);
  }



  template <typename Device>
  VISKORES_CONT void SetNumChannels(const viskores::Int32 numChannels, Device)
  {
    if (numChannels < 1)
    {
      std::string msg = "ChannelBuffer SetNumChannels: numBins must be greater that 0";
      throw viskores::cont::ErrorBadValue(msg);
    }
    if (this->NumChannels == numChannels)
      return;

    this->NumChannels = numChannels;
    viskores::cont::Token token;
    this->Buffer.PrepareForOutput(this->Size * this->NumChannels, Device(), token);
  }

  template <typename Device>
  VISKORES_CONT void Resize(const viskores::Id newSize, Device device)
  {
    if (newSize < 0)
      throw viskores::cont::ErrorBadValue("ChannelBuffer resize: Size must be greater than -1 ");
    this->Size = newSize;
    viskores::cont::Token token;
    this->Buffer.PrepareForOutput(
      this->Size * static_cast<viskores::Id>(NumChannels), device, token);
  }
};
}
}
} // namespace viskores::rendering::raytracing

#endif
