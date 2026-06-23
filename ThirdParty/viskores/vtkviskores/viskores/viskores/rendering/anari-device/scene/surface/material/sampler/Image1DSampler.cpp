//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Image1DSampler.h"
// Viskores
#include <viskores/TypeTraits.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandleConstant.h>
// std
#include <limits>

namespace
{

template <typename T>
constexpr T opaqueValue(viskores::TypeTraitsRealTag)
{
  return T(1);
}
template <typename T>
constexpr T opaqueValue(viskores::TypeTraitsIntegerTag)
{
  return std::numeric_limits<T>::max();
}
template <typename T>
constexpr T opaqueValue()
{
  return opaqueValue<T>(typename viskores::TypeTraits<T>::NumericTag{});
}

template <typename T>
constexpr viskores::Float32 floatConvert(T anariValue, viskores::TypeTraitsRealTag)
{
  return static_cast<viskores::Float32>(anariValue);
}
template <typename T>
constexpr viskores::Float32 floatConvert(T anariValue, viskores::TypeTraitsIntegerTag)
{
  return static_cast<viskores::Float32>(anariValue) /
    static_cast<viskores::Float32>(std::numeric_limits<T>::max());
}
template <typename T>
constexpr viskores::Float32 floatConvert(T anariValue)
{
  return floatConvert(anariValue, typename viskores::TypeTraits<T>::NumericTag{});
}

template <typename ComponentType>
bool captureColorTableType(const viskores::cont::UnknownArrayHandle& colorArray,
                           viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap)
{
  const viskores::Id numValues = colorArray.GetNumberOfValues();
  std::array<viskores::cont::ArrayHandleStride<ComponentType>, 4> colorChannelArrays;
  viskores::cont::ArrayHandleConstant<ComponentType> ones(opaqueValue<ComponentType>(), numValues);
  switch (colorArray.GetNumberOfComponentsFlat())
  {
    case 1:
      // Grayscale color
      colorChannelArrays[0] = colorChannelArrays[1] = colorChannelArrays[2] =
        colorArray.ExtractComponent<ComponentType>(0);
      colorChannelArrays[3] = viskores::cont::ArrayExtractComponent(ones, 0);
      break;
    case 2:
      // Grayscale + alpha
      colorChannelArrays[0] = colorChannelArrays[1] = colorChannelArrays[2] =
        colorArray.ExtractComponent<ComponentType>(0);
      colorChannelArrays[3] = colorArray.ExtractComponent<ComponentType>(1);
      break;
    case 3:
      // RGB
      colorChannelArrays[0] = colorArray.ExtractComponent<ComponentType>(0);
      colorChannelArrays[1] = colorArray.ExtractComponent<ComponentType>(1);
      colorChannelArrays[2] = colorArray.ExtractComponent<ComponentType>(2);
      colorChannelArrays[3] = viskores::cont::ArrayExtractComponent(ones, 0);
      break;
    case 4:
      // RGBA
      colorChannelArrays[0] = colorArray.ExtractComponent<ComponentType>(0);
      colorChannelArrays[1] = colorArray.ExtractComponent<ComponentType>(1);
      colorChannelArrays[2] = colorArray.ExtractComponent<ComponentType>(2);
      colorChannelArrays[3] = colorArray.ExtractComponent<ComponentType>(3);
      break;
    default:
      return false;
  }
  std::array<typename viskores::cont::ArrayHandleStride<ComponentType>::ReadPortalType, 4>
    colorChannelPortals;
  for (viskores::IdComponent channel = 0; channel < 4; ++channel)
  {
    colorChannelPortals[channel] = colorChannelArrays[channel].ReadPortal();
  }
  colorMap.Allocate(numValues);
  viskores::cont::ArrayHandle<viskores::Vec4f_32>::WritePortalType colorMapPortal =
    colorMap.WritePortal();
  for (viskores::Id sample = 0; sample < numValues; ++sample)
  {
    colorMapPortal.Set(sample,
                       { floatConvert(colorChannelPortals[0].Get(sample)),
                         floatConvert(colorChannelPortals[1].Get(sample)),
                         floatConvert(colorChannelPortals[2].Get(sample)),
                         floatConvert(colorChannelPortals[3].Get(sample)) });
  }
  return true;
}

bool captureColorTable(const viskores::cont::UnknownArrayHandle& colorArray,
                       viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap)
{
  if (colorArray.IsBaseComponentType<viskores::UInt8>())
  {
    return captureColorTableType<viskores::UInt8>(colorArray, colorMap);
  }
  else if (colorArray.IsBaseComponentType<viskores::UInt16>())
  {
    return captureColorTableType<viskores::UInt16>(colorArray, colorMap);
  }
  else if (colorArray.IsBaseComponentType<viskores::UInt32>())
  {
    return captureColorTableType<viskores::UInt32>(colorArray, colorMap);
  }
  else if (colorArray.IsBaseComponentType<viskores::Float32>())
  {
    return captureColorTableType<viskores::Float32>(colorArray, colorMap);
  }
  else
  {
    return false;
  }
}

} // anonymous namespace
namespace viskores_device
{

Image1DSampler::Image1DSampler(ViskoresDeviceGlobalState* d)
  : Sampler(d)
  , m_colorArray(this)
{
}

void Image1DSampler::commitParameters()
{
  this->Sampler::commitParameters();

  this->m_inAttribute = this->getParamString("inAttribute", "attribute0");

  mat4 inTransform = this->getParam("inTransform", mat4(linalg::identity));
  this->m_inTransform = toViskoresMatrix(inTransform);
  float4 inOffset = this->getParam("inOffset", float4(0.f, 0.f, 0.f, 0.f));
  this->m_inOffset = { inOffset[0], inOffset[1], inOffset[2], inOffset[3] };

  this->m_colorArray = this->getParamObject<Array1D>("image");

  this->m_wrapMode = helium::wrapModeFromString(this->getParamString("wrapMode", "clampToEdge"));
  if (this->m_wrapMode == helium::WrapMode::DEFAULT)
  {
    this->m_wrapMode = helium::WrapMode::CLAMP_TO_EDGE;
  }
}

void Image1DSampler::finalize()
{
  this->Sampler::finalize();

  bool colorMapFilled = false;
  if (this->m_colorArray)
  {
    // TODO: This method does not properly handle SRGB.
    colorMapFilled = captureColorTable(this->m_colorArray->dataAsViskoresArray(), this->m_colorMap);
    if (!colorMapFilled)
    {
      this->reportMessage(ANARI_SEVERITY_WARNING,
                          "color array provided for image1D sampling has unrecognized type");
    }
  }

  if (!colorMapFilled)
  {
    this->reportMessage(ANARI_SEVERITY_WARNING,
                        "image1D sampling requested, but no color array given");
    this->m_colorMap.Allocate(1);
    this->m_colorMap.WritePortal().Set(0, { 1, 1, 1, 1 });
  }
}

bool Image1DSampler::getColors(const viskores::cont::DataSet& data,
                               viskores::cont::Field& field,
                               viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const
{
  if (!data.HasField(this->inAttribute()))
  {
    this->reportMessage(
      ANARI_SEVERITY_WARNING, "sampler attribute %s not found", this->inAttribute().c_str());
    return false;
  }

  viskores::cont::Field attribField = data.GetField(this->inAttribute());
  viskores::cont::UnknownArrayHandle attribArray = attribField.GetData();
  if (!attribArray.CanConvert<viskores::cont::ArrayHandle<viskores::Float32>>())
  {
    if (!attribArray.IsBaseComponentType<viskores::Float32>())
    {
      this->reportMessage(ANARI_SEVERITY_WARNING,
                          "attribute array type not currently supported for image1D sampler.");
      return false;
    }
    this->reportMessage(ANARI_SEVERITY_PERFORMANCE_WARNING,
                        "todo: handle vector attributes more efficiently");
    viskores::cont::UnknownArrayHandle newArray;
    viskores::cont::ArrayCopy(attribArray.ExtractComponent<viskores::Float32>(0), newArray);
    attribArray = newArray;
  }

  field = viskores::cont::Field{ attribField.GetName(), attribField.GetAssociation(), attribArray };
  colorMap = this->m_colorMap;
  return true;
}

} // namespace viskores_device
