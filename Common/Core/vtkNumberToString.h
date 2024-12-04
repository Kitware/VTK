// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkNumberToString
 * @brief Convert floating and fixed point numbers to strings
 *
 * This class uses the double-conversion library to convert float and double
 * numbers to std::string without numerical precision errors.
 * You can use specify the output format using SetNotation to either Mixed,
 * Scientific or Fixed. In Mixed mode (default),
 * it is possible to specify the low and high exponent where the string representation
 * will switch to scientific notation instead of fixed point notation.
 *
 * Unless specified using `SetPrecision`, the formatted value will not have trailing zeroes.
 *
 * For types other than float and double, this class relies on std::to_string.
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
 *
 *  @code{cpp}
 *  #include "vtkNumberToString.h"
 *  double a = 4.2;
 *  vtkNumberToString converter;
 *  converter.SetNotation(Scientific);
 *  converter.SetPrecision(4);
 *  std::cout << converter.Convert(a) << std::endl;
 * @endcode

 */
#ifndef vtkNumberToString_h
#define vtkNumberToString_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkTypeTraits.h"

#include <ostream>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkNumberToString
{
public:
  ///@{
  /**
   * Set/Get the LowExponent for string conversion.
   * It correspond to the closest to zero exponent value that
   * will use fixed point notation in the returned string instead of a scientific notation.
   * Only used when Notation value is Mixed (default).
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
   * Only used when Notation value is Mixed (default).
   * HighExponent = 6, 1e6 -> "1000000"
   * HighExponent = 5, 1e6 -> "1e6"
   */
  void SetHighExponent(int highExponent);
  int GetHighExponent();
  ///@}

  enum Notation
  {
    Mixed,
    Scientific,
    Fixed
  };

  ///@{
  /**
   * Set/Get the notation used for string conversion.
   * Mixed (0) will choose between fixed-point and scientific notation
   * depending on HighExponent and LowExponent.
   * Scientific (1) will always use scientific notation
   * Fixed (2) will always use fixed-point notation.
   * Note that Fixed can't be used for values that have more than 60 digits either
   * before or after the decimal point.
   * Default is 0 (Mixed)
   */
  void SetNotation(int notation);
  int GetNotation();
  ///@}

  ///@{
  /**
   * Set/Get the floating-point precision used for string conversion.
   * The precision specifies the number of decimal places to display for
   * Scientific and Fixed-point notations.
   * In Mixed mode, this parameter is not used, and the string will display as many decimal places
   * as needed in order not to have any trailing zeroes and keep full precision.
   * Default is 2.
   */
  void SetPrecision(int precision);
  int GetPrecision();
  ///@}

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

private:
  int LowExponent = -6;
  int HighExponent = 20;
  int Notation = Mixed;
  int Precision = 2;
};

VTKCOMMONCORE_EXPORT ostream& operator<<(ostream& stream, const vtkNumberToString::TagDouble& tag);
VTKCOMMONCORE_EXPORT ostream& operator<<(ostream& stream, const vtkNumberToString::TagFloat& tag);

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkNumberToString.h
