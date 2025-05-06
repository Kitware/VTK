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

#ifndef viskores_rendering_Texture2D_h
#define viskores_rendering_Texture2D_h

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ExecutionObjectBase.h>

namespace viskores
{
namespace rendering
{

enum class TextureFilterMode
{
  NearestNeighbour,
  Linear,
}; // enum TextureFilterMode

enum class TextureWrapMode
{
  Clamp,
  Repeat,
}; // enum TextureWrapMode

template <viskores::IdComponent NumComponents>
class Texture2D
{
public:
  using TextureDataHandle = typename viskores::cont::ArrayHandle<viskores::UInt8>;
  using ColorType = viskores::Vec<viskores::Float32, NumComponents>;

  class Texture2DSampler;

#define UV_BOUNDS_CHECK(u, v, NoneType)             \
  if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) \
  {                                                 \
    return NoneType();                              \
  }

  VISKORES_CONT
  Texture2D()
    : Width(0)
    , Height(0)
  {
  }

  VISKORES_CONT
  Texture2D(viskores::Id width, viskores::Id height, const TextureDataHandle& data)
    : Width(width)
    , Height(height)
    , FilterMode(TextureFilterMode::Linear)
    , WrapMode(TextureWrapMode::Clamp)
  {
    VISKORES_ASSERT(data.GetNumberOfValues() == (Width * Height * NumComponents));
    // We do not know the lifetime of the underlying data source of input `data`. Since it might
    // be from a shallow copy of the data source, we make a deep copy of the input data and keep
    // it's portal. The copy operation is very fast.
    this->Data.DeepCopyFrom(data);
  }

  VISKORES_CONT
  bool IsValid() const { return Width > 0 && Height > 0; }

  VISKORES_CONT
  TextureFilterMode GetFilterMode() const { return this->FilterMode; }

  VISKORES_CONT
  void SetFilterMode(TextureFilterMode filterMode) { this->FilterMode = filterMode; }

  VISKORES_CONT
  TextureWrapMode GetWrapMode() const { return this->WrapMode; }

  VISKORES_CONT
  void SetWrapMode(TextureWrapMode wrapMode) { this->WrapMode = wrapMode; }

  VISKORES_CONT Texture2DSampler GetExecObjectFactory() const
  {
    return Texture2DSampler(Width, Height, Data, FilterMode, WrapMode);
  }

  template <typename Device>
  class Texture2DSamplerExecutionObject
  {
  public:
    using TextureExecPortal = typename TextureDataHandle::ReadPortalType;

    VISKORES_CONT
    Texture2DSamplerExecutionObject()
      : Width(0)
      , Height(0)
    {
    }

    VISKORES_CONT
    Texture2DSamplerExecutionObject(viskores::Id width,
                                    viskores::Id height,
                                    const TextureDataHandle& data,
                                    TextureFilterMode filterMode,
                                    TextureWrapMode wrapMode,
                                    viskores::cont::Token& token)
      : Width(width)
      , Height(height)
      , Data(data.PrepareForInput(Device(), token))
      , FilterMode(filterMode)
      , WrapMode(wrapMode)
    {
    }

    VISKORES_EXEC
    inline ColorType GetColor(viskores::Float32 u, viskores::Float32 v) const
    {
      v = 1.0f - v;
      UV_BOUNDS_CHECK(u, v, ColorType);
      switch (FilterMode)
      {
        case TextureFilterMode::NearestNeighbour:
          return GetNearestNeighbourFilteredColor(u, v);

        case TextureFilterMode::Linear:
          return GetLinearFilteredColor(u, v);

        default:
          return ColorType();
      }
    }

  private:
    VISKORES_EXEC
    inline ColorType GetNearestNeighbourFilteredColor(viskores::Float32 u,
                                                      viskores::Float32 v) const
    {
      viskores::Id x =
        static_cast<viskores::Id>(viskores::Round(u * static_cast<viskores::Float32>(Width - 1)));
      viskores::Id y =
        static_cast<viskores::Id>(viskores::Round(v * static_cast<viskores::Float32>(Height - 1)));
      return GetColorAtCoords(x, y);
    }

    VISKORES_EXEC
    inline ColorType GetLinearFilteredColor(viskores::Float32 u, viskores::Float32 v) const
    {
      u = u * static_cast<viskores::Float32>(Width) - 0.5f;
      v = v * static_cast<viskores::Float32>(Height) - 0.5f;
      viskores::Id x = static_cast<viskores::Id>(viskores::Floor(u));
      viskores::Id y = static_cast<viskores::Id>(viskores::Floor(v));
      viskores::Float32 uRatio = u - static_cast<viskores::Float32>(x);
      viskores::Float32 vRatio = v - static_cast<viskores::Float32>(y);
      viskores::Float32 uOpposite = 1.0f - uRatio;
      viskores::Float32 vOpposite = 1.0f - vRatio;
      viskores::Id xn, yn;
      GetNextCoords(x, y, xn, yn);
      ColorType c1 = GetColorAtCoords(x, y);
      ColorType c2 = GetColorAtCoords(xn, y);
      ColorType c3 = GetColorAtCoords(x, yn);
      ColorType c4 = GetColorAtCoords(xn, yn);
      return (c1 * uOpposite + c2 * uRatio) * vOpposite + (c3 * uOpposite + c4 * uRatio) * vRatio;
    }

    VISKORES_EXEC
    inline ColorType GetColorAtCoords(viskores::Id x, viskores::Id y) const
    {
      viskores::Id idx = (y * Width + x) * NumComponents;
      ColorType color;
      for (viskores::IdComponent i = 0; i < NumComponents; ++i)
      {
        color[i] = Data.Get(idx + i) / 255.0f;
      }
      return color;
    }

    VISKORES_EXEC
    inline void GetNextCoords(viskores::Id x,
                              viskores::Id y,
                              viskores::Id& xn,
                              viskores::Id& yn) const
    {
      switch (WrapMode)
      {
        case TextureWrapMode::Clamp:
          xn = (x + 1) < Width ? (x + 1) : x;
          yn = (y + 1) < Height ? (y + 1) : y;
          break;
        case TextureWrapMode::Repeat:
        default:
          xn = (x + 1) % Width;
          yn = (y + 1) % Height;
          break;
      }
    }

    viskores::Id Width;
    viskores::Id Height;
    TextureExecPortal Data;
    TextureFilterMode FilterMode;
    TextureWrapMode WrapMode;
  };

  class Texture2DSampler : public viskores::cont::ExecutionObjectBase
  {
  public:
    VISKORES_CONT
    Texture2DSampler()
      : Width(0)
      , Height(0)
    {
    }

    VISKORES_CONT
    Texture2DSampler(viskores::Id width,
                     viskores::Id height,
                     const TextureDataHandle& data,
                     TextureFilterMode filterMode,
                     TextureWrapMode wrapMode)
      : Width(width)
      , Height(height)
      , Data(data)
      , FilterMode(filterMode)
      , WrapMode(wrapMode)
    {
    }

    template <typename Device>
    VISKORES_CONT Texture2DSamplerExecutionObject<Device> PrepareForExecution(
      Device,
      viskores::cont::Token& token) const
    {
      return Texture2DSamplerExecutionObject<Device>(
        this->Width, this->Height, this->Data, this->FilterMode, this->WrapMode, token);
    }

  private:
    viskores::Id Width;
    viskores::Id Height;
    TextureDataHandle Data;
    TextureFilterMode FilterMode;
    TextureWrapMode WrapMode;
  }; // class Texture2DSampler

private:
  viskores::Id Width;
  viskores::Id Height;
  TextureDataHandle Data;
  TextureFilterMode FilterMode;
  TextureWrapMode WrapMode;
}; // class Texture2D
}
} // namespace viskores::rendering

#endif // viskores_rendering_Texture2D_h
