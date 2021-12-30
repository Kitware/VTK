/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoordinateFrame.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCoordinateFrame.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <cmath>

vtkStandardNewMacro(vtkCoordinateFrame);

double vtkCoordinateFrame::EvaluateFunction(double x[3])
{
  using std::signbit;
  using std::sqrt;
  double val = 0.;
  // dw is a vector in world coordinates from the coordinate frame's origin to x.
  vtkVector3d dw(x[0] - this->Origin[0], x[1] - this->Origin[1], x[2] - this->Origin[2]);
  vtkVector3d ax(this->XAxis);
  vtkVector3d ay(this->YAxis);
  vtkVector3d az(this->ZAxis);
  // Transform dw into the coordinate-frame's basis:
  double xx = dw.Dot(ax);
  double yy = dw.Dot(ay);
  double zz = dw.Dot(az);
  // Compute squares of each coordinate and squared-radius from origin.
  double x2 = xx * xx;
  double y2 = yy * yy;
  double z2 = zz * zz;
  double r2 = x2 + y2 + z2;
  // Evaluate 2 relevant quartic spherical harmonic basis functions, Y_4,0 and Y_4,4:
  constexpr double c40 = 0.10578554691520431; // = 3/16.0 * std::sqrt(1. / vtkMath::Pi());
  constexpr double c44 = 0.62583573544917614; // = 3/16.0 * std::sqrt(35. / vtkMath::Pi());
  double Y40 = c40 * (35 * z2 * z2 - 30 * z2 * r2 + 3 * r2 * r2) / r2 / r2;
  double Y44 = c44 * (x2 * (x2 - 3 * y2) - y2 * (3 * x2 - y2)) / r2 / r2;
  // Combine the basis functions to get our coordinate-frame function:
  constexpr double w40 = 0.76376261582597338; // = std::sqrt(7./12.);
  constexpr double w44 = 0.64549722436790280; // = std::sqrt(5./12.);
  val = w40 * Y40 + w44 * Y44;
  return val;
}

// Evaluate planes gradient.
void vtkCoordinateFrame::EvaluateGradient(double x[3], double n[3])
{
  // Approximate derivate. TODO: Compute analytic gradient.
  vtkVector3d pp(x[0], x[1], x[2]);
  double fxyz = this->EvaluateFunction(pp.GetData());
  double fxdx = this->EvaluateFunction((pp + 1e-6 * vtkVector3d(1, 0, 0)).GetData());
  double fydy = this->EvaluateFunction((pp + 1e-6 * vtkVector3d(0, 1, 0)).GetData());
  double fzdz = this->EvaluateFunction((pp + 1e-6 * vtkVector3d(0, 0, 1)).GetData());
  // vtkVector3d grad((fxdx - fxyz)/1e-6, (fydy - fxyz)/1e-6, (fzdz - fxyz)/1e-6);
  n[0] = (fxdx - fxyz) / 1e-6;
  n[1] = (fydy - fxyz) / 1e-6;
  n[2] = (fzdz - fxyz) / 1e-6;
}

void vtkCoordinateFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Origin: " << this->Origin[0] << " " << this->Origin[1] << " " << this->Origin[2]
     << "\n";
  os << indent << "XAxis: " << this->XAxis[0] << " " << this->XAxis[1] << " " << this->XAxis[2]
     << "\n";
  os << indent << "YAxis: " << this->YAxis[0] << " " << this->YAxis[1] << " " << this->YAxis[2]
     << "\n";
  os << indent << "ZAxis: " << this->ZAxis[0] << " " << this->ZAxis[1] << " " << this->ZAxis[2]
     << "\n";
}
