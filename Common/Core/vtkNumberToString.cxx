// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNumberToString.h"

// clang-format off
#include "vtk_doubleconversion.h"
#include VTK_DOUBLECONVERSION_HEADER(double-conversion.h)
// clang-format on

#include <array>
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

//------------------------------------------------------------------------------
std::string vtkNumberToString::Convert(double val)
{
  // Copied from double-conversion::EcmaScriptConverter
  // the last two arguments of the constructor have no effect with ToShortest
  constexpr int flags = double_conversion::DoubleToStringConverter::UNIQUE_ZERO |
    double_conversion::DoubleToStringConverter::EMIT_POSITIVE_EXPONENT_SIGN;
  double_conversion::DoubleToStringConverter converter(
    flags, "Infinity", "NaN", 'e', this->LowExponent, this->HighExponent + 1, 6, 0);

  std::array<char, 256> buf;
  double_conversion::StringBuilder builder(buf.data(), static_cast<int>(buf.size()));
  builder.Reset();

  if (this->GetNotation() == Scientific)
  {
    converter.ToExponential(val, this->Precision, &builder);
  }
  else if (this->GetNotation() == Fixed)
  {
    converter.ToFixed(val, this->Precision, &builder);
  }
  else
  {
    converter.ToShortest(val, &builder);
  }
  return builder.Finalize();
}

//------------------------------------------------------------------------------
std::string vtkNumberToString::Convert(float val)
{
  // Copied from double-conversion::EcmaScriptConverter
  // the last two arguments of the constructor have no effect with ToShortest
  constexpr int flags = double_conversion::DoubleToStringConverter::UNIQUE_ZERO |
    double_conversion::DoubleToStringConverter::EMIT_POSITIVE_EXPONENT_SIGN;
  double_conversion::DoubleToStringConverter converter(
    flags, "Infinity", "NaN", 'e', this->LowExponent, this->HighExponent + 1, 6, 0);

  std::array<char, 256> buf;
  double_conversion::StringBuilder builder(buf.data(), static_cast<int>(buf.size()));
  builder.Reset();

  if (this->GetNotation() == Scientific)
  {
    converter.ToExponential(val, this->Precision, &builder);
  }
  else if (this->GetNotation() == Fixed)
  {
    converter.ToFixed(val, this->Precision, &builder);
  }
  else
  {
    converter.ToShortestSingle(val, &builder);
  }
  return builder.Finalize();
}

VTK_ABI_NAMESPACE_END
