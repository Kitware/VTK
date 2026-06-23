//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "ArrayConversion.h"
#include "anari/frontend/type_utility.h"
// Viskores
#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>
// C++
#include <limits>
#include <type_traits>


namespace
{

template <typename BaseType, int NumComponents>
struct ViskoresTypeImpl
{
  using type = viskores::Vec<BaseType, static_cast<viskores::IdComponent>(NumComponents)>;
};
template <typename BaseType>
struct ViskoresTypeImpl<BaseType, 1>
{
  using type = BaseType;
};

template <int ANARIDataTypeId>
struct ANARIToViskoresTypeImpl
{
  using properties = anari::ANARITypeProperties<ANARIDataTypeId>;
  using type =
    typename ViskoresTypeImpl<typename properties::base_type, properties::components>::type;
};

template <int ANARIDataTypeId>
using ANARIToViskoresType = typename ANARIToViskoresTypeImpl<ANARIDataTypeId>::type;

template <int ANARIDataTypeId>
struct ConstructArrayHandle
{
  using T = ANARIToViskoresType<ANARIDataTypeId>;
  viskores::cont::UnknownArrayHandle operator()(const void* memory, viskores::Id numValues)
  {
    return this->DoIt(memory, numValues, std::is_pointer<T>{});
  }

  viskores::cont::UnknownArrayHandle DoIt(const void* memory,
                                          viskores::Id numValues,
                                          std::false_type /*is_pointer*/)
  {
    // TODO: The ANARI interface generally passes arrays in the host memory space.
    // This can be really inefficient as data is pulled from GPU to CPU, passed to
    // ANARI, and then copied right back to the GPU. Need an extension to allow
    // client applications to pass device memory directly.
    return viskores::cont::make_ArrayHandle(
      reinterpret_cast<const T*>(memory), numValues, viskores::CopyFlag::Off);
  }

  viskores::cont::UnknownArrayHandle DoIt(const void*, viskores::Id, std::true_type /*is_pointer*/)
  {
    std::cout << "Cannot convert an array of type "
              << anari::ANARITypeProperties<ANARIDataTypeId>::type_name << " to Viskores.\n";
    return viskores::cont::UnknownArrayHandle{};
  }
};
template <>
struct ConstructArrayHandle<ANARI_UNKNOWN>
{
  viskores::cont::UnknownArrayHandle operator()(const void*, viskores::Id)
  {
    std::cout << "Cannot convert an array of type ANARI_UNKNOWN to Viskores.\n";
    return viskores::cont::UnknownArrayHandle{};
  }
};


struct ConvertColorValues : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  template <typename InType, typename OutType>
  VISKORES_EXEC void operator()(const InType& inValue, OutType& outValue) const
  {
    using InComponentType = typename InType::ComponentType;
    using OutComponentType = typename OutType::ComponentType;

    constexpr OutComponentType scale = OutComponentType{ 1 } /
      static_cast<OutComponentType>(std::numeric_limits<InComponentType>::max());
    for (viskores::IdComponent index = 0; index < inValue.GetNumberOfComponents(); ++index)
    {
      outValue[index] = static_cast<OutComponentType>(inValue[index]) * scale;
    }
  }
};

template <typename T>
inline void FixColorsForType(viskores::cont::UnknownArrayHandle& colorArray)
{
  using ArrayType = viskores::cont::ArrayHandle<viskores::Vec<T, 4>>;
  if (colorArray.CanConvert<ArrayType>())
  {
    viskores::cont::ArrayHandle<viskores::Vec4f> retypedArray;
    viskores::cont::Invoker invoke;
    invoke(ConvertColorValues{}, colorArray.AsArrayHandle<ArrayType>(), retypedArray);
    colorArray = retypedArray;
  }
}

} // anonymous namespace

namespace viskores_device
{

viskores::cont::UnknownArrayHandle ANARIArrayToViskoresArray(const helium::Array* anariArray)
{
  viskores::Id numValues = anariArray->totalSize();
  const void* memory = anariArray->data();

  return anari::anariTypeInvoke<viskores::cont::UnknownArrayHandle, ConstructArrayHandle>(
    anariArray->elementType(), memory, numValues);
}

viskores::cont::UnknownArrayHandle ANARIColorsToViskoresColors(
  const viskores::cont::UnknownArrayHandle& anariColors)
{
  viskores::cont::UnknownArrayHandle viskoresColors = anariColors;
  FixColorsForType<viskores::UInt8>(viskoresColors);
  FixColorsForType<viskores::UInt16>(viskoresColors);
  FixColorsForType<viskores::UInt32>(viskoresColors);
  FixColorsForType<viskores::UInt64>(viskoresColors);
  return viskoresColors;
}

} // namespace viskores_device
