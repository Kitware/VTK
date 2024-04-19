// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @file vtkMultiProcessStreamSerialization.h
 * @brief Utility to serialize STL containers to vtkMultiProcessStream
 *
 * This header provides helpers to make it easier to serialize STL containers
 * to vtkMultiProcessStream.
 *
 * Typical usage is as follows:
 *
 * @code{.cpp}
 *
 * std::set<std::string> set_of_strings;
 * ...
 * vtkMultiProcessStream stream;
 * stream << set_of_strings; // save
 *
 * .... do communication ...
 *
 * vtkMultiProcessStream result;
 * stream >> result; // load
 *
 * ...
 * @endcode
 */

#ifndef vtkMultiProcessSerialization_h
#define vtkMultiProcessSerialization_h

#include "vtkMultiProcessStream.h"

#include <array>
#include <map>
#include <set>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
template <typename T>
struct Serialization
{
  static void Save(vtkMultiProcessStream& stream, const T& t) { stream << t; }
  static void Load(vtkMultiProcessStream& stream, T& t) { stream >> t; }
};

template <typename ElementType>
struct Serialization<std::set<ElementType>>
{
  static void Save(vtkMultiProcessStream& stream, const std::set<ElementType>& set)
  {
    stream << static_cast<vtkTypeInt64>(set.size());
    for (const auto& elem : set)
    {
      Serialization<ElementType>::Save(stream, elem);
    }
  }

  static void Load(vtkMultiProcessStream& stream, std::set<ElementType>& set)
  {
    vtkTypeInt64 count;
    stream >> count;
    for (vtkTypeInt64 cc = 0; cc < count; ++cc)
    {
      ElementType elem;
      Serialization<ElementType>::Load(stream, elem);
      set.insert(std::move(elem));
    }
  }
};

template <typename ElementType, std::size_t N>
struct Serialization<std::array<ElementType, N>>
{
  static void Save(vtkMultiProcessStream& stream, const std::array<ElementType, N>& array)
  {
    for (const auto& elem : array)
    {
      Serialization<ElementType>::Save(stream, elem);
    }
  }

  static void Load(vtkMultiProcessStream& stream, std::array<ElementType, N>& array)
  {
    for (std::size_t cc = 0; cc < N; ++cc)
    {
      Serialization<ElementType>::Load(stream, array[cc]);
    }
  }
};

template <typename T1, typename T2>
struct Serialization<std::pair<T1, T2>>
{
  static void Save(vtkMultiProcessStream& stream, const std::pair<T1, T2>& pair)
  {
    Serialization<T1>::Save(stream, pair.first);
    Serialization<T2>::Save(stream, pair.second);
  }

  static void Load(vtkMultiProcessStream& stream, std::pair<T1, T2>& pair)
  {
    Serialization<T1>::Load(stream, pair.first);
    Serialization<T2>::Load(stream, pair.second);
  }
};

template <typename T1, typename T2>
struct Serialization<std::map<T1, T2>>
{
  static void Save(vtkMultiProcessStream& stream, const std::map<T1, T2>& map)
  {
    stream << static_cast<vtkTypeInt64>(map.size());
    for (const auto& pair : map)
    {
      Serialization<std::pair<T1, T2>>::Save(stream, pair);
    }
  }

  static void Load(vtkMultiProcessStream& stream, std::map<T1, T2>& map)
  {
    vtkTypeInt64 count;
    stream >> count;
    for (vtkTypeInt64 cc = 0; cc < count; ++cc)
    {
      std::pair<T1, T2> pair;
      Serialization<std::pair<T1, T2>>::Load(stream, pair);
      map.insert(std::move(pair));
    }
  }
};

template <typename ElementType>
struct Serialization<std::vector<ElementType>>
{
  static void Save(vtkMultiProcessStream& stream, const std::vector<ElementType>& vector)
  {
    stream << static_cast<vtkTypeInt64>(vector.size());
    for (const auto& elem : vector)
    {
      Serialization<ElementType>::Save(stream, elem);
    }
  }

  static void Load(vtkMultiProcessStream& stream, std::vector<ElementType>& vector)
  {
    vtkTypeInt64 count;
    stream >> count;
    for (vtkTypeInt64 cc = 0; cc < count; ++cc)
    {
      ElementType elem;
      Serialization<ElementType>::Load(stream, elem);
      vector.push_back(std::move(elem));
    }
  }
};

template <typename T>
inline vtkMultiProcessStream& operator<<(vtkMultiProcessStream& stream, const T& value)
{
  Serialization<T>::Save(stream, value);
  return stream;
}

template <typename T>
inline vtkMultiProcessStream& operator>>(vtkMultiProcessStream& stream, T& value)
{
  Serialization<T>::Load(stream, value);
  return stream;
}

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkMultiProcessStreamSerialization.h
