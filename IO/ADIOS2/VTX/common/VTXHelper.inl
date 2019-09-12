/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXHelper.inl

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXHelper.inl
 *
 *  Created on: May 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_VTX_COMMON_VTXHelper_inl
#define VTK_IO_ADIOS2_VTX_COMMON_VTXHelper_inl

#include <algorithm>
#include <iostream>
#include <sstream>

namespace vtx
{
namespace helper
{

template<class T>
std::vector<T> StringToVector(const std::string& input) noexcept
{
  std::vector<T> output;
  std::istringstream inputSS(input);

  T record;
  while (inputSS >> record)
  {
    output.push_back(record);
  }
  return output;
}

template<class T, class U>
std::vector<T> MapKeysToVector(const std::map<T, U>& input) noexcept
{
  std::vector<T> keys;
  keys.reserve(input.size());

  for (const auto& pair : input)
  {
    keys.push_back(pair.first);
  }
  return keys;
}

template<class T>
void Print(const std::vector<T>& input, const std::string& name)
{
  std::ostringstream oss;
  size_t i = 0;

  oss << name << " = { ";
  for (const T in : input)
  {
    oss << in << ", ";
    ++i;
  }
  oss << "}  rank : " << MPIGetRank();
  std::cout << oss.str() << "\n";
}

} // end namespace helper
} // end namespace vtx

#endif /* VTK_IO_ADIOS2_VTX_COMMON_VTXHelper_inl */
