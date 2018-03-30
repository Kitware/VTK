/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNumberToString.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkNumberToString
 * @brief Convert floating and fixed point numbers to strings
 *
 * This class uses the double-conversion library to convertfloating point and
 * fixed point numbers to ASCII versions that are represented without
 * numerical precision errors.
 *
 * Typical use:
 *  \#include "vtkNumberToString.h"
 *  vtkNumberToString<float> convert;
 *  float a = 1.0f/3.0f;
 *  std::cout << convert(a) << std::endl;
 *
 */
#ifndef vtkNumberToString_h
#define vtkNumberToString_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkTypeTraits.h"

#include <sstream>
#include <string>

std::string VTKIOCORE_EXPORT vtkNumberToStringImplementation(double val);
std::string VTKIOCORE_EXPORT vtkNumberToStringImplementation(float val);

template <typename T>
class VTKIOCORE_EXPORT vtkNumberToString
{
public:
  std::string operator()(T val)
  {
    std::ostringstream output;
    output << static_cast<typename vtkTypeTraits<T>::PrintType>(val);
    return output.str();
  }

private:
  void operator=(const vtkNumberToString&) = delete;
};

template <>
inline std::string vtkNumberToString<double>::operator()(double val)
{
  return vtkNumberToStringImplementation(val);
}
template <>
inline std::string vtkNumberToString<float>::operator()(float val)
{
  return vtkNumberToStringImplementation(val);
}

#endif
// VTK-HeaderTest-Exclude: vtkNumberToString.h
