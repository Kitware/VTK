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
#ifndef viskores_cont_SerializableTypeString_h
#define viskores_cont_SerializableTypeString_h

#include <viskores/Types.h>

#include <string>

namespace viskores
{
namespace cont
{

/// \brief A traits class that gives a unique name for a type. This class
/// should be specialized for every type that has to be serialized by diy.
template <typename T>
struct SerializableTypeString
#ifdef VISKORES_DOXYGEN_ONLY
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "TypeName";
    return name;
  }
}
#endif
;

namespace internal
{

template <typename T, typename... Ts>
std::string GetVariadicSerializableTypeString(const T&, const Ts&... ts)
{
  return SerializableTypeString<T>::Get() + "," + GetVariadicSerializableTypeString(ts...);
}

template <typename T>
std::string GetVariadicSerializableTypeString(const T&)
{
  return SerializableTypeString<T>::Get();
}

} // internal

/// @cond SERIALIZATION
template <>
struct SerializableTypeString<viskores::Int8>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "I8";
    return name;
  }
};

template <>
struct SerializableTypeString<viskores::UInt8>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "U8";
    return name;
  }
};

template <>
struct SerializableTypeString<viskores::Int16>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "I16";
    return name;
  }
};

template <>
struct SerializableTypeString<viskores::UInt16>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "U16";
    return name;
  }
};

template <>
struct SerializableTypeString<viskores::Int32>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "I32";
    return name;
  }
};

template <>
struct SerializableTypeString<viskores::UInt32>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "U32";
    return name;
  }
};

template <>
struct SerializableTypeString<viskores::Int64>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "I64";
    return name;
  }
};

template <>
struct SerializableTypeString<viskores::UInt64>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "U64";
    return name;
  }
};

template <>
struct SerializableTypeString<viskores::Float32>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "F32";
    return name;
  }
};

template <>
struct SerializableTypeString<viskores::Float64>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "F64";
    return name;
  }
};

template <>
struct SerializableTypeString<bool>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "B8";
    return name;
  }
};

template <>
struct SerializableTypeString<char>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "C8";
    return name;
  }
};

template <>
struct SerializableTypeString<VISKORES_UNUSED_INT_TYPE>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "L" + std::to_string(sizeof(VISKORES_UNUSED_INT_TYPE) * 8);
    return name;
  }
};

template <>
struct SerializableTypeString<unsigned VISKORES_UNUSED_INT_TYPE>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "UL" + std::to_string(sizeof(unsigned VISKORES_UNUSED_INT_TYPE) * 8);
    return name;
  }
};

template <typename T, viskores::IdComponent NumComponents>
struct SerializableTypeString<viskores::Vec<T, NumComponents>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name =
      "V<" + SerializableTypeString<T>::Get() + "," + std::to_string(NumComponents) + ">";
    return name;
  }
};

template <typename T1, typename T2>
struct SerializableTypeString<viskores::Pair<T1, T2>>
{
  static VISKORES_CONT const std::string& Get()
  {
    static std::string name = "viskores::Pair<" + SerializableTypeString<T1>::Get() + "," +
      SerializableTypeString<T2>::Get() + ">";
    return name;
  }
};
}
} // viskores::cont
/// @endcond SERIALIZATION

#endif // viskores_cont_SerializableTypeString_h
