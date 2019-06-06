/*=========================================================================

 Program:   Visualization Toolkit
 Module:    ADIOS2Helper.inl

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * ADIOS2Helper.inl
 *
 *  Created on: May 3, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef VTK_IO_ADIOS2_ADIOS2HELPER_INL_
#define VTK_IO_ADIOS2_ADIOS2HELPER_INL_

#include <algorithm>
#include <iostream>
#include <sstream>

namespace adios2vtk
{
namespace helper
{

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
} // end namespace adios2vtk

#endif /* VTK_IO_ADIOS2_ADIOS2HELPER_INL_ */
