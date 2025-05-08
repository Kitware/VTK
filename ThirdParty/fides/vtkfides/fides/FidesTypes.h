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

#include <fides/Deprecated.h>

#include <string>
#include <unordered_map>

#include <viskores/cont/Field.h>

#include "fides_export.h"

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

namespace fides
{

/// Parameters for an individual data source, e.g., Parameters needed
/// by ADIOS for configuring an Engine.
using DataSourceParams = std::unordered_map<std::string, std::string>;

/// Parameters for all data sources mapped to their source name.
/// The key must match the name given for the data source in the JSON file.
using Params = std::unordered_map<std::string, DataSourceParams>;

/// Possible return values when using Fides in a streaming mode
enum class StepStatus
{
  OK,
  NotReady,
  EndOfStream
};

/// Association for fields, based on Viskores's association enum, but
/// also includes a value for representing field data.
enum class FIDES_DEPRECATED(
  1.1,
  "fides::Association is no longer used. Use viskores::cont::Field::Association directly.")
  Association
{
  POINTS,
  CELL_SET,
  FIELD_DATA
};

/// Converts an fides::Association to a viskores::cont::Field::Association.
/// Throws a runtime error if trying to convert fides::Association::FIELD_DATA
FIDES_DEPRECATED_SUPPRESS_BEGIN
FIDES_DEPRECATED(
  1.1,
  "fides::Association is no longer used. Use viskores::cont::Field::Association directly.")
viskores::cont::Field::Association FIDES_EXPORT
ConvertToViskoresAssociation(fides::Association assoc);
FIDES_DEPRECATED_SUPPRESS_END

/// Converts viskores::cont::Field::Association to fides::Association.
/// Throws an error if assoc is not either POINTS or CELL_SET
FIDES_DEPRECATED_SUPPRESS_BEGIN
FIDES_DEPRECATED(
  1.1,
  "fides::Association is no longer used. Use viskores::cont::Field::Association directly.")
fides::Association FIDES_EXPORT
ConvertViskoresAssociationToFides(viskores::cont::Field::Association assoc);
FIDES_DEPRECATED_SUPPRESS_END

/// Converts a Viskores cell shape type to the fides string.
/// Throws a runtime error for unsupported cell types.
std::string FIDES_EXPORT ConvertViskoresCellTypeToFides(viskores::UInt8 cellShapeType);

/// Converts a fides cell name to Viskores cell shape type.
/// Throws a runtime error for unsupported cell types.
viskores::UInt8 FIDES_EXPORT ConvertFidesCellTypeToViskores(const std::string& cellShapeName);

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
