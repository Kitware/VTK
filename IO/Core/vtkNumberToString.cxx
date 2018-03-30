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

#include "vtk_doubleconversion.h"
#include VTK_DOUBLECONVERSION_HEADER(double - conversion.h)

#include <sstream>

//----------------------------------------------------------------------------
std::string vtkNumberToStringImplementation(double val)
{
  char buf[256];
  const double_conversion::DoubleToStringConverter& converter =
    double_conversion::DoubleToStringConverter::EcmaScriptConverter();
  double_conversion::StringBuilder builder(buf, sizeof(buf));
  builder.Reset();
  converter.ToShortest(val, &builder);
  return std::string(builder.Finalize());
}

//----------------------------------------------------------------------------
std::string vtkNumberToStringImplementation(float val)
{
  char buf[256];
  const double_conversion::DoubleToStringConverter& converter =
    double_conversion::DoubleToStringConverter::EcmaScriptConverter();

  double_conversion::StringBuilder builder(buf, sizeof(buf));
  builder.Reset();
  converter.ToShortestSingle(val, &builder);
  return std::string(builder.Finalize());
}
