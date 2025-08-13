// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNumberToString.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
template <typename TagT>
inline ostream& ToString(ostream& stream, const TagT& tag)
{
  vtkNumberToString converter;
  stream << converter.Convert(tag.Value);
  return stream;
}
}

//------------------------------------------------------------------------------
ostream& operator<<(ostream& stream, const vtkNumberToString::TagDouble& tag)
{
  return ToString(stream, tag);
}

//------------------------------------------------------------------------------
ostream& operator<<(ostream& stream, const vtkNumberToString::TagFloat& tag)
{
  return ToString(stream, tag);
}

//------------------------------------------------------------------------------
void vtkNumberToString::SetLowExponent(int lowExponent)
{
  this->LowExponent = lowExponent;
}

//------------------------------------------------------------------------------
int vtkNumberToString::GetLowExponent()
{
  return this->LowExponent;
}

//------------------------------------------------------------------------------
void vtkNumberToString::SetHighExponent(int highExponent)
{
  this->HighExponent = highExponent;
}

//------------------------------------------------------------------------------
int vtkNumberToString::GetHighExponent()
{
  return this->HighExponent;
}

//------------------------------------------------------------------------------
void vtkNumberToString::SetNotation(int notation)
{
  this->Notation = notation;
}

//------------------------------------------------------------------------------
int vtkNumberToString::GetNotation()
{
  return this->Notation;
}

//------------------------------------------------------------------------------
void vtkNumberToString::SetPrecision(int precision)
{
  this->Precision = precision;
}

//------------------------------------------------------------------------------
int vtkNumberToString::GetPrecision()
{
  return this->Precision;
}

// Helper function to remove trailing zeros in scientific notation (e.g., "1.23000e+05" ->
// "1.23e+05")
static std::string RemoveTrailingZerosScientific(const std::string_view& str)
{
  const auto e_pos = str.find('e');
  if (e_pos == std::string::npos)
  {
    return std::string(str); // Not in scientific notation
  }
  const auto dot_pos = str.find('.');
  if (dot_pos == std::string::npos || dot_pos >= e_pos)
  {
    return std::string(str); // No decimal part or malformed scientific format
  }
  // Find last non-zero digit before 'e'
  size_t last_non_zero = e_pos;
  while (last_non_zero > dot_pos && str[last_non_zero - 1] == '0')
  {
    --last_non_zero;
  }
  // If only the dot is left (e.g., "1.000e+05" -> "1e+05")
  if (last_non_zero == dot_pos + 1)
  {
    --last_non_zero;
  }
  // Build the final string efficiently
  std::string result;
  result.reserve(str.size()); // optional, for performance
  result.append(str.substr(0, last_non_zero));
  result.append(str.substr(e_pos));
  return result;
}

//------------------------------------------------------------------------------
template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
static std::string ConvertFloat(
  const int& notation, const int& precision, const int& lowExponent, const int& highExponent, T val)
{
  // Handle special cases
  if (std::isinf(val))
  {
    return val > 0 ? "Infinity" : "-Infinity";
  }
  if (std::isnan(val))
  {
    return "NaN";
  }
  switch (notation)
  {
    case vtkNumberToString::Scientific:
    {
      // Scientific notation
      return vtk::format(FMT_STRING("{:.{}e}"), val, precision);
    }
    case vtkNumberToString::Fixed:
    {
      // Fixed notation
      return vtk::format(FMT_STRING("{:.{}f}"), val, precision);
    }
    case vtkNumberToString::Mixed:
    default:
    {
      const int exponent = val != 0.0 ? static_cast<int>(std::floor(std::log10(std::abs(val)))) : 0;
      if (exponent <= lowExponent || exponent >= highExponent)
      {
        // Scientific notation
        static constexpr int maxDigits = std::numeric_limits<T>::max_digits10;
        // Use maximum precision to ensure accuracy, and remove trailing zeros
        return RemoveTrailingZerosScientific(vtk::format(FMT_STRING("{:.{}e}"), val, maxDigits));
      }
      // Shortest notation
      return vtk::to_string(val);
    }
  }
}

//------------------------------------------------------------------------------
std::string vtkNumberToString::Convert(double val)
{
  return ConvertFloat(this->Notation, this->Precision, this->LowExponent, this->HighExponent, val);
}

//------------------------------------------------------------------------------
std::string vtkNumberToString::Convert(float val)
{
  return ConvertFloat(this->Notation, this->Precision, this->LowExponent, this->HighExponent, val);
}

VTK_ABI_NAMESPACE_END
