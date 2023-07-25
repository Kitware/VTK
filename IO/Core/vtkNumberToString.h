// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkNumberToString
 * @brief Convert floating and fixed point numbers to strings
 *
 * This class uses the double-conversion library to convert float and double
 * numbers to std::string without numerical precision errors.
 * It is possible to specify the low and high exponent where the string representation
 * will switch to scientific notation instead of fixed point notation.
 *
 * For other types, this class rely on std::to_string.
 *
 * Typical use:
 *
 * @code{cpp}
 *  #include "vtkNumberToString.h"
 *  float a = 1.0f/3.0f;
 *  vtkNumberToString converter;
 *  std::cout << converter.Convert(a) << std::endl;
 * @endcode
 *
 * @code{cpp}
 *  #include "vtkNumberToString.h"
 *  double a = 1e7*vtkMath::PI();
 *  vtkNumberToString converter;
 *  converter.SetLowExponent(-6);
 *  converter.SetHighExponent(6);
 *  std::cout << converter.Convert(a) << std::endl;
 * @endcode

 */
#ifndef vtkNumberToString_h
#define vtkNumberToString_h

#include "vtkDeprecation.h"  // For VTK_DEPRECATED_IN_9_3_0
#include "vtkIOCoreModule.h" // For export macro
#include "vtkTypeTraits.h"

#include <ostream>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCORE_EXPORT vtkNumberToString
{
public:
  ///@{
  /**
   * Set/Get the LowExponent for string conversion.
   * It correspond to the closest to zero exponent value that
   * will use fixed point notation in the returned string instead of a scientific notation.
   * eg:
   * LowExponent = 6, 1e-6 -> "0.000001"
   * LowExponent = 5, 1e-6 -> "1e-6"
   */
  void SetLowExponent(int lowExponent);
  int GetLowExponent();
  ///@}

  ///@{
  /**
   * Set/Get the HighExponent for string conversion.
   * HighExponent correspond to the highest exponent value that
   * will use fixed point notation in the returned string instead of a scientific notation.
   * HighExponent = 6, 1e6 -> "1000000"
   * HighExponent = 5, 1e6 -> "1e6"
   */
  void SetHighExponent(int highExponent);
  int GetHighExponent();

  ///@{
  /**
   * Convert a number to an accurate string representation of that number.
   * A templated generic implementation is provided, which rely on std::to_string for types
   * other than double or float.
   */
  std::string Convert(double val);
  std::string Convert(float val);
  template <typename T>
  std::string Convert(const T& val)
  {
    return std::to_string(val);
  }
  ///@}

  struct TagDouble
  {
    double Value;
    TagDouble(const double& value)
      : Value(value)
    {
    }
  };

  struct TagFloat
  {
    float Value;
    TagFloat(const float& value)
      : Value(value)
    {
    }
  };

  template <typename T>
  const T& operator()(const T& val) const
  {
    return val;
  }
  VTK_DEPRECATED_IN_9_3_0("Use vtkNumberToString::Convert instead.")
  TagDouble operator()(const double& val) const { return TagDouble(val); }
  VTK_DEPRECATED_IN_9_3_0("Use vtkNumberToString::Convert instead.")
  TagFloat operator()(const float& val) const { return TagFloat(val); }

private:
  int LowExponent = -6;
  int HighExponent = 20;
};

VTKIOCORE_EXPORT ostream& operator<<(ostream& stream, const vtkNumberToString::TagDouble& tag);
VTKIOCORE_EXPORT ostream& operator<<(ostream& stream, const vtkNumberToString::TagFloat& tag);

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkNumberToString.h
