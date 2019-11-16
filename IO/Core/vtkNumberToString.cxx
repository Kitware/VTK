/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNumberToString.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNumberToString.h"

// clang-format off
#include "vtk_doubleconversion.h"
#include VTK_DOUBLECONVERSION_HEADER(double-conversion.h)
// clang-format on

#include <sstream>

namespace
{
template <typename TagT>
inline ostream& ToString(ostream& stream, const TagT& tag)
{
  char buf[256];
  const double_conversion::DoubleToStringConverter& converter =
    double_conversion::DoubleToStringConverter::EcmaScriptConverter();
  double_conversion::StringBuilder builder(buf, sizeof(buf));
  builder.Reset();
  converter.ToShortest(tag.Value, &builder);
  stream << builder.Finalize();
  return stream;
}
}

//----------------------------------------------------------------------------
ostream& operator<<(ostream& stream, const vtkNumberToString::TagDouble& tag)
{
  return ToString(stream, tag);
}

//----------------------------------------------------------------------------
ostream& operator<<(ostream& stream, const vtkNumberToString::TagFloat& tag)
{
  return ToString(stream, tag);
}
