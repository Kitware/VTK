// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCoordinateFrame.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

template <class A>
bool fuzzyCompare(const A& a, const A& b, const std::string& msg, double tol = 1e-4);

template <>
bool fuzzyCompare<double>(const double& a, const double& b, const std::string& msg, double tol)
{
  if (std::abs(a - b) > tol)
  {
    throw std::logic_error("Failed comparison " + msg);
  }
  return true;
}

template <>
bool fuzzyCompare<vtkVector3d>(
  const vtkVector3d& a, const vtkVector3d& b, const std::string& msg, double tol)
{
  if (std::abs((b - a).Norm()) > tol)
  {
    throw std::logic_error("Failed comparison " + msg);
  }
  return true;
}

int TestCoordinateFrame(int, char*[])
{
  try
  {
    vtkSmartPointer<vtkCoordinateFrame> frame = vtkSmartPointer<vtkCoordinateFrame>::New();
    frame->SetOrigin(0.0, 0.0, 0.0);
    fuzzyCompare(vtkVector3d(frame->GetXAxis()), vtkVector3d(1, 0, 0), "Invalid X initialization");
    fuzzyCompare(vtkVector3d(frame->GetYAxis()), vtkVector3d(0, 1, 0), "Invalid Y initialization");
    fuzzyCompare(vtkVector3d(frame->GetZAxis()), vtkVector3d(0, 0, 1), "Invalid Z initialization");
    std::array<vtkVector3d, 6> testPoints{ vtkVector3d(1, 0, 0), vtkVector3d(0, 1, 0),
      vtkVector3d(0, 0, 1), vtkVector3d(1, 1, 1), vtkVector3d(-1, 1, 1), vtkVector3d(0, -10, -10) };
    std::array<double, 6> testValues{ {
      0.64636,
      0.64636,
      0.64636,
      -0.430907,
      -0.430907,
      -0.16159,
    } };
    vtkVector3d grad;
    for (std::size_t ii = 0; ii < 6; ++ii)
    {
      double value = frame->EvaluateFunction(testPoints[ii].GetData());
      frame->EvaluateGradient(testPoints[ii].GetData(), grad.GetData());
      double fxdx = frame->EvaluateFunction((testPoints[ii] + vtkVector3d(1e-06, 0, 0)).GetData());
      double fydy = frame->EvaluateFunction((testPoints[ii] + vtkVector3d(0, 1e-06, 0)).GetData());
      double fzdz = frame->EvaluateFunction((testPoints[ii] + vtkVector3d(0, 0, 1e-06)).GetData());
      vtkVector3d agrad((fxdx - value) / 1e-06, (fydy - value) / 1e-06, (fzdz - value) / 1e-06);
      std::cout << testPoints[ii] << ": " << value << " grad " << grad << " agrad " << agrad
                << "\n";
      std::ostringstream msg;
      msg << ii << ": Expected f(" << testPoints[ii] << ") = " << testValues[ii] << ", got "
          << value << ".";
      fuzzyCompare(value, testValues[ii], msg.str());
      fuzzyCompare(grad, agrad, msg.str());
    }
    /* Uncomment to get a polar plot of the z=0 slice.
    for (std::size_t ii = 0; ii < 60; ++ii)
    {
      double theta = ii * 0.10471975511965977;
      double value = frame->EvaluateFunction(vtkVector3d(std::cos(theta), std::sin(theta),
    0).GetData()); std::cout << theta << ", " << value << "\n";
    }
    */
  }
  catch (std::logic_error& e)
  {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
