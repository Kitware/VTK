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
#ifndef viskores_io_internal_VTKDataSetTypes_h
#define viskores_io_internal_VTKDataSetTypes_h

#include <viskores/Types.h>
#include <viskores/VecTraits.h>

#include <algorithm>
#include <cassert>
#include <string>

namespace viskores
{
namespace io
{
namespace internal
{

enum DataType
{
  DTYPE_UNKNOWN = 0,
  DTYPE_BIT,
  DTYPE_UNSIGNED_CHAR,
  DTYPE_CHAR,
  DTYPE_UNSIGNED_SHORT,
  DTYPE_SHORT,
  DTYPE_UNSIGNED_INT,
  DTYPE_INT,
  DTYPE_UNSIGNED_LONG,
  DTYPE_LONG,
  DTYPE_FLOAT,
  DTYPE_DOUBLE,
  DTYPE_UNSIGNED_LONG_LONG,
  DTYPE_LONG_LONG,

  DTYPE_COUNT
};

inline const char* DataTypeString(int id)
{
  static const char* strings[] = {
    "",      "bit",          "unsigned_char", "char",          "unsigned_short",
    "short", "unsigned_int", "int",           "unsigned_long", "long",
    "float", "double",       "vtktypeuint64", "vtktypeint64"
  };
  return strings[id];
}

inline DataType DataTypeId(const std::string& str)
{
  DataType type = DTYPE_UNKNOWN;
  for (int id = 1; id < DTYPE_COUNT; ++id)
  {
    if (str == DataTypeString(id))
    {
      type = static_cast<DataType>(id);
    }
  }

  return type;
}

struct DummyBitType
{
  // Needs to work with streams' << operator
  operator bool() const { return false; }
};

class ColorChannel8
{
public:
  ColorChannel8()
    : Data()
  {
  }
  ColorChannel8(viskores::UInt8 val)
    : Data(val)
  {
  }
  ColorChannel8(viskores::Float32 val)
    : Data(static_cast<viskores::UInt8>(std::min(std::max(val, 1.0f), 0.0f) * 255))
  {
  }
  operator viskores::Float32() const { return static_cast<viskores::Float32>(this->Data) / 255.0f; }
  operator viskores::UInt8() const { return this->Data; }

private:
  viskores::UInt8 Data;
};

inline std::ostream& operator<<(std::ostream& out, const ColorChannel8& val)
{
  return out << static_cast<viskores::Float32>(val);
}

inline std::istream& operator>>(std::istream& in, ColorChannel8& val)
{
  viskores::Float32 fval;
  in >> fval;
  val = ColorChannel8(fval);
  return in;
}

template <typename T>
struct DataTypeName
{
  static const char* Name() { return "unknown"; }
};
template <>
struct DataTypeName<DummyBitType>
{
  static const char* Name() { return "bit"; }
};
template <>
struct DataTypeName<viskores::Int8>
{
  static const char* Name() { return "char"; }
};
template <>
struct DataTypeName<viskores::UInt8>
{
  static const char* Name() { return "unsigned_char"; }
};
template <>
struct DataTypeName<viskores::Int16>
{
  static const char* Name() { return "short"; }
};
template <>
struct DataTypeName<viskores::UInt16>
{
  static const char* Name() { return "unsigned_short"; }
};
template <>
struct DataTypeName<viskores::Int32>
{
  static const char* Name() { return "int"; }
};
template <>
struct DataTypeName<viskores::UInt32>
{
  static const char* Name() { return "unsigned_int"; }
};
template <>
struct DataTypeName<viskores::Int64>
{
  static const char* Name() { return "long"; }
};
template <>
struct DataTypeName<viskores::UInt64>
{
  static const char* Name() { return "unsigned_long"; }
};
template <>
struct DataTypeName<viskores::Float32>
{
  static const char* Name() { return "float"; }
};
template <>
struct DataTypeName<viskores::Float64>
{
  static const char* Name() { return "double"; }
};

template <typename Functor>
inline void SelectTypeAndCall(DataType dtype, Functor&& functor)
{
  switch (dtype)
  {
    case DTYPE_BIT:
      functor(DummyBitType());
      break;
    case DTYPE_UNSIGNED_CHAR:
      functor(viskores::UInt8());
      break;
    case DTYPE_CHAR:
      functor(viskores::Int8());
      break;
    case DTYPE_UNSIGNED_SHORT:
      functor(viskores::UInt16());
      break;
    case DTYPE_SHORT:
      functor(viskores::Int16());
      break;
    case DTYPE_UNSIGNED_INT:
      functor(viskores::UInt32());
      break;
    case DTYPE_INT:
      functor(viskores::Int32());
      break;
    case DTYPE_UNSIGNED_LONG:
    case DTYPE_UNSIGNED_LONG_LONG:
      functor(viskores::UInt64());
      break;
    case DTYPE_LONG:
    case DTYPE_LONG_LONG:
      functor(viskores::Int64());
      break;
    case DTYPE_FLOAT:
      functor(viskores::Float32());
      break;
    case DTYPE_DOUBLE:
      functor(viskores::Float64());
      break;
    default:
      assert(false);
  }
}

}
}
} // namespace viskores::io::internal

#endif // viskores_io_internal_VTKDataSetTypes_h
