//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_FidesTypes_H_
#define fides_FidesTypes_H_

#include <fides/DataContainer.h>
#include <fides/FidesOptions.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <fides/fides_export.h>

#define fidesNotUsed(parameter_name)

// copied from ADIOS2/source/adios2/common/ADIOSMacros.h
// Kind of annoying, but it is really helpful for dealing
// with the fact that ADIOS stores types in strings and keeps us from having
// to duplicate code by hand for each type
#define FIDES_FOREACH_ATTRIBUTE_PRIMITIVE_STDTYPE_1ARG(MACRO) \
  MACRO(int8_t)                                               \
  MACRO(int16_t)                                              \
  MACRO(int32_t)                                              \
  MACRO(int64_t)                                              \
  MACRO(uint8_t)                                              \
  MACRO(uint16_t)                                             \
  MACRO(uint32_t)                                             \
  MACRO(uint64_t)                                             \
  MACRO(float)                                                \
  MACRO(double)

#define FIDES_FOREACH_STDTYPE_1ARG(MACRO) \
  MACRO(std::string)                      \
  FIDES_FOREACH_PRIMITIVE_STDTYPE_1ARG(MACRO)

// Forward declarations for aliases
#if FIDES_USE_VISKORES
namespace viskores
{
namespace cont
{
class PartitionedDataSet;
}
}
#endif

#if FIDES_USE_VTK
#include <vtkABINamespace.h>
VTK_ABI_NAMESPACE_BEGIN
template <typename T>
class vtkSmartPointer;
class vtkPartitionedDataSet;
VTK_ABI_NAMESPACE_END
#endif

#if FIDES_USE_CONDUIT
namespace conduit
{
class Node;
}
#endif

namespace fides
{

// Depending on support, use either type aliasing or opaque stubs
#if FIDES_USE_VISKORES
using ViskoresCollection = viskores::cont::PartitionedDataSet;
#else
struct ViskoresMissingStub
{
};
using ViskoresCollection = ViskoresMissingStub;
#endif

#if FIDES_USE_VTK
using VTKCollection = vtkSmartPointer<vtkPartitionedDataSet>;
#else
struct VTKMissingStub
{
};
using VTKCollection = VTKMissingStub;
#endif

#if FIDES_USE_CONDUIT
using ConduitNode = conduit::Node;
using ConduitCollection = std::shared_ptr<conduit::Node>;
#else
struct ConduitMissingStub
{
};
using ConduitNode = ConduitMissingStub;
using ConduitCollection = ConduitMissingStub;
#endif

/// Public output extractors
FIDES_EXPORT ViskoresCollection GetAsViskoresPDS(DataContainer& container);
FIDES_EXPORT VTKCollection GetAsVTKPDS(DataContainer& container);
FIDES_EXPORT ConduitCollection GetAsConduit(DataContainer& container);

/// Public input wrappers
FIDES_EXPORT std::shared_ptr<DataContainer> Wrap(const ViskoresCollection& dataset);
FIDES_EXPORT std::shared_ptr<DataContainer> Wrap(const VTKCollection& dataset);
FIDES_EXPORT std::shared_ptr<DataContainer> Wrap(const ConduitCollection& node);
FIDES_EXPORT std::shared_ptr<DataContainer> WrapExternal(ConduitNode* node);

/// Methods returning whether Fides was compiled with support for the
/// various optional backends
bool HasVTKSupport();
bool HasViskoresSupport();
bool HasConduitSupport();

/// Parameters for an individual data source, e.g., Parameters needed
/// by ADIOS for configuring an Engine.
using DataSourceParams = std::unordered_map<std::string, std::string>;

/// Parameters for all data sources mapped to their source name.
/// The key must match the name given for the data source in the JSON file.
using Params = std::unordered_map<std::string, DataSourceParams>;

enum class DataSetType
{
  VTK,
  Viskores
};

/// Possible return values when using Fides in a streaming mode
enum class StepStatus
{
  OK,
  NotReady,
  EndOfStream,
  OtherError
};

/// Fides-owned field association enum. Used in the output builder interface
/// so that core code does not depend on Viskores or VTK enums.
///
/// \c CellGrid identifies attributes that live on a vtkCellGrid (DG finite
/// element data); they don't fit the points/cells/whole-dataset triad and need
/// to be surfaced separately so consumers can offer their own selection UI for
/// them.
enum class FieldAssociation
{
  Points,
  Cells,
  WholeDataSet,
  CellGrid
};

/// Fides-owned cell shape enum. Values match VTK and Viskores cell type constants
/// so that conversion is a simple cast rather than a per-cell switch.
enum class CellShape : uint8_t
{
  Vertex = 1,
  Line = 3,
  Triangle = 5,
  Quad = 9,
  Tetra = 10,
  Hexahedron = 12,
  Wedge = 13,
  Pyramid = 14
};

/// Fides-owned data type enum for RawArray.
enum class DataType
{
  Float32,
  Float64,
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64
};

/// Returns the size in bytes of a single element of the given DataType.
inline size_t SizeOfDataType(DataType type)
{
  switch (type)
  {
    case DataType::Float32:
      return sizeof(float);
    case DataType::Float64:
      return sizeof(double);
    case DataType::Int8:
      return sizeof(int8_t);
    case DataType::Int16:
      return sizeof(int16_t);
    case DataType::Int32:
      return sizeof(int32_t);
    case DataType::Int64:
      return sizeof(int64_t);
    case DataType::UInt8:
      return sizeof(uint8_t);
    case DataType::UInt16:
      return sizeof(uint16_t);
    case DataType::UInt32:
      return sizeof(uint32_t);
    case DataType::UInt64:
      return sizeof(uint64_t);
    default:
      throw std::runtime_error("Unknown DataType");
  }
}

/// Returns the DataType enum corresponding to the C++ type T.
/// Uses if-constexpr to handle platform-specific type aliases
/// (e.g., char, long, unsigned long may be distinct from stdint types).
template <typename T>
inline DataType GetDataType()
{
  if constexpr (std::is_same_v<T, float>)
    return DataType::Float32;
  else if constexpr (std::is_same_v<T, double>)
    return DataType::Float64;
  else if constexpr (std::is_integral_v<T>)
  {
    if constexpr (std::is_signed_v<T>)
    {
      if constexpr (sizeof(T) == 1)
        return DataType::Int8;
      else if constexpr (sizeof(T) == 2)
        return DataType::Int16;
      else if constexpr (sizeof(T) == 4)
        return DataType::Int32;
      else if constexpr (sizeof(T) == 8)
        return DataType::Int64;
    }
    else
    {
      if constexpr (sizeof(T) == 1)
        return DataType::UInt8;
      else if constexpr (sizeof(T) == 2)
        return DataType::UInt16;
      else if constexpr (sizeof(T) == 4)
        return DataType::UInt32;
      else if constexpr (sizeof(T) == 8)
        return DataType::UInt64;
    }
  }
}

/// A neutral buffer type for exchanging array data between DataSource
/// and the output builder layer. Replaces viskores::cont::UnknownArrayHandle
/// in the core data flow.
struct FIDES_EXPORT RawArray
{
  /// Refcounted buffer. null for deferred reads (inline engine).
  std::shared_ptr<void> Data;

  /// Number of elements (not bytes).
  size_t NumValues = 0;

  /// 1 for scalar, 2 or 3 for vector components.
  int NumComponents = 1;

  /// Element data type.
  DataType Type = DataType::Float32;

  RawArray() = default;

  RawArray(std::shared_ptr<void> data, size_t numValues, int numComponents, DataType type)
    : Data(std::move(data))
    , NumValues(numValues)
    , NumComponents(numComponents)
    , Type(type)
  {
  }

  /// Typed pointer access. Caller must ensure T matches Type.
  template <typename T>
  const T* GetPointer() const
  {
    return static_cast<const T*>(Data.get());
  }

  /// Typed mutable pointer access.
  template <typename T>
  T* GetWritePointer()
  {
    return static_cast<T*>(Data.get());
  }

  /// Get a single element value. Caller must ensure T matches Type and index is valid.
  template <typename T>
  T GetValue(size_t index) const
  {
    return GetPointer<T>()[index];
  }

  /// Total size in bytes.
  size_t GetNumberOfBytes() const
  {
    return NumValues * static_cast<size_t>(NumComponents) * SizeOfDataType(Type);
  }

  /// Whether this RawArray has data.
  bool IsValid() const { return Data != nullptr && NumValues > 0; }
};

/// Helper to allocate a RawArray of the given type and size.
template <typename T>
RawArray AllocateRawArray(size_t numValues, int numComponents = 1)
{
  size_t totalElements = numValues * static_cast<size_t>(numComponents);
  std::shared_ptr<void> buf(new T[totalElements], [](void* p) { delete[] static_cast<T*>(p); });
  return RawArray(std::move(buf), numValues, numComponents, fides::GetDataType<T>());
}

/// Converts a fides cell name string to a CellShape enum value.
/// Throws a runtime error for unsupported cell types.
CellShape FIDES_EXPORT ConvertStringToCellShape(const std::string& cellShapeName);

/// Converts a CellShape enum to its fides string name.
std::string FIDES_EXPORT ConvertCellShapeToString(CellShape shape);

/// Returns the number of vertices per cell for the given CellShape.
int FIDES_EXPORT GetCellShapeNumberOfPoints(CellShape shape);

/// Converts a DataType enum to its ADIOS2-compatible type string.
std::string FIDES_EXPORT ConvertDataTypeToString(DataType type);

// used with the type macros above
template <class T>
std::string GetType();

template <>
inline std::string GetType<std::string>()
{
  return "string";
}

template <>
inline std::string GetType<int8_t>()
{
  return "int8_t";
}
template <>
inline std::string GetType<uint8_t>()
{
  return "uint8_t";
}
template <>
inline std::string GetType<int16_t>()
{
  return "int16_t";
}
template <>
inline std::string GetType<uint16_t>()
{
  return "uint16_t";
}
template <>
inline std::string GetType<int32_t>()
{
  return "int32_t";
}
template <>
inline std::string GetType<uint32_t>()
{
  return "uint32_t";
}
template <>
inline std::string GetType<int64_t>()
{
  return "int64_t";
}
template <>
inline std::string GetType<uint64_t>()
{
  return "uint64_t";
}
template <>
inline std::string GetType<float>()
{
  return "float";
}
template <>
inline std::string GetType<double>()
{
  return "double";
}

}

#endif
