/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParametricSpline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkParametricSpline.h"
#include "vtkCardinalSpline.h"
#include "vtkKochanekSpline.h"
#include "vtkPoints.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"

#include <math.h>

#include "vtkTestErrorObserver.h"

void TestPrint();
int TestSetGet();
int TestConstraints();
int TestErrors();
int TestMisc();
int TestSetPoints(vtkPoints *, bool closed = false);
void GeneratePoints(int, vtkSmartPointer<vtkPoints>&);
void GenerateRandomPoints(int, vtkSmartPointer<vtkPoints>&);
int UnitTestParametricSpline(int,char *[])
{
  int status = 0;
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  GeneratePoints(100, points);
  TestPrint();
  status += TestErrors();
  status += TestSetGet();
  status += TestConstraints();
  status += TestSetPoints(points);
  status += TestSetPoints(points, true);
  status += TestMisc();

  return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

void GeneratePoints(int npts, vtkSmartPointer<vtkPoints> &points)
{
  points->SetNumberOfPoints(npts);
  double deltaX = vtkMath::Pi() * 2.0 / (npts - 1);
  double deltaZ = 2.0 / (npts - 1);
  for (int n = 0; n < npts; ++n)
    {
    double x[3];
    x[0] = -vtkMath::Pi() + n * deltaX;
    x[1] = std::sin(x[0]);
    x[2] = -1.0 + n * deltaZ;
    points->SetPoint(n, x[0], x[1], x[2]);
    }
}

int TestSetPoints(vtkPoints *points, bool closed)
{
  int status = 0;

  vtkSmartPointer<vtkParametricSpline> pspline =
    vtkSmartPointer<vtkParametricSpline>::New();
  if (closed)
    {
    pspline->ClosedOn();
    }
  else
    {
    pspline->ClosedOff();
    }
  int npts =  points->GetNumberOfPoints();

  pspline->SetNumberOfPoints(npts);

  double length = 0.0;

  vtkPoints* knots = vtkPoints::New(VTK_DOUBLE);
  knots->SetNumberOfPoints(npts);

  double xm1[3] = {0.0,0.0,0.0};
  double x[3];
  for (int n = 0; n < npts; ++n)
    {
    points->GetPoint(n, x);
    pspline->SetPoint(n, x[0], x[1], x[2]);
    if (n > 0)
      {
      length += std::sqrt(vtkMath::Distance2BetweenPoints(x, xm1));
      }
    xm1[0] = x[0];
    xm1[1] = x[1];
    xm1[2] = x[2];
    }
  if (closed)
    {
    points->GetPoint(0, x);
    points->GetPoint(npts - 1, xm1);
    length += std::sqrt(vtkMath::Distance2BetweenPoints(x, xm1));
    }

  double tolerance = 4.0*std::numeric_limits<double>::epsilon();
  pspline->ParameterizeByLengthOff();
  for (int n = 0; n < npts; ++n)
    {
    double t[3];
    if (closed)
      {
      t[0] = static_cast<double>(n) / static_cast<double>((npts));
      }
    else
      {
      t[0] = static_cast<double>(n) / static_cast<double>((npts - 1));
      }
    t[1] = t[2] = 0.0;

    points->GetPoint(n, x);

    double result[3];
    pspline->Evaluate(t, result, NULL);
    if (!vtkMathUtilities::FuzzyCompare(x[0], result[0], tolerance) ||
        !vtkMathUtilities::FuzzyCompare(x[1], result[1], tolerance) ||
        !vtkMathUtilities::FuzzyCompare(x[2], result[2], tolerance))
      {
      std::cout << "TestSetPoints(by point id): Expected "
                << x[0] << ", " << x[1] << ", " << x[2]
                << " but got "
                << result[0] << ", " << result[1] << ", " << result[2]
                << std::endl;
      ++status;
      }
    }

  // Test with externally created points
  tolerance = 8.0*std::numeric_limits<double>::epsilon();
  pspline->SetPoints(points);
  pspline->ParameterizeByLengthOn();
  double totalLength = length;

  length = 0.0;
  for (int n = 0; n < npts; ++n)
    {
    points->GetPoint(n, x);

    if (n > 0)
      {
      length += std::sqrt(vtkMath::Distance2BetweenPoints(x, xm1));
      }
    double t[3];
    t[0] = length / totalLength;
    t[1] = t[2] = 0.0;

    double result[3];
    pspline->Evaluate(t, result, NULL);
    if (!vtkMathUtilities::FuzzyCompare(x[0], result[0], tolerance) ||
        !vtkMathUtilities::FuzzyCompare(x[1], result[1], tolerance) ||
        !vtkMathUtilities::FuzzyCompare(x[2], result[2], tolerance))
      {
      std::cout << "TestSetPoints(by length): Expected "
                << x[0] << ", " << x[1] << ", " << x[2]
                << " but got "
                << result[0] << ", " << result[1] << ", " << result[2]
                << std::endl;
      ++status;
      }
    xm1[0] = x[0];
    xm1[1] = x[1];
    xm1[2] = x[2];
    }

  knots->Delete();
  return status;
}

int TestErrors()
{
  int status = 0;
  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  vtkSmartPointer<vtkParametricSpline> pspline =
    vtkSmartPointer<vtkParametricSpline>::New();
  pspline->AddObserver(vtkCommand::ErrorEvent, errorObserver);

  double x[3];
  x[0] = 0.0; x[1] = 0.0; x[2] = 0.0;
  double result[3];

  pspline->Evaluate(x, result, 0);
  // Check for model bounds error
  if (errorObserver->GetError())
    {
    std::cout << "Caught expected error: "
              << errorObserver->GetErrorMessage();
    }
  else
    {
    std::cout << "Failed to catch expected 'Please specify points' error" << std::endl;
    ++status;
    }
  errorObserver->Clear();

  pspline->SetNumberOfPoints(0);
  pspline->EvaluateScalar(x, 0, 0);
  if (errorObserver->GetError())
    {
    std::cout << "Caught expected error: "
              << errorObserver->GetErrorMessage();
    }
  else
    {
    std::cout << "Failed to catch expected 'Please specify at least one point' error" << std::endl;
    ++status;
    }
  errorObserver->Clear();

  pspline->SetXSpline(NULL);
  pspline->Evaluate(x, result, 0);
  if (errorObserver->GetError())
    {
    std::cout << "Caught expected error: "
              << errorObserver->GetErrorMessage();
    }
  else
    {
    std::cout << "Failed to catch expected 'Please specify spline' error" << std::endl;
    ++status;
    }
  errorObserver->Clear();

  return status;
}

int TestSetGet()
{
  int status = 0;
  vtkSmartPointer<vtkParametricSpline> pspline =
    vtkSmartPointer<vtkParametricSpline>::New();

  if (pspline->GetDimension() != 1)
    {
    std::cout << "GetDimension: expected 1 but got "
              << pspline->GetDimension()<< std::endl;
    ++status;
    }

  if (pspline->GetParameterizeByLength() != 1)
    {
    std::cout << "GetParameterizeByLength: expected 1 but got "
              << pspline->GetParameterizeByLength()<< std::endl;
    ++status;
    }

  if (pspline->GetPoints() != NULL)
    {
    std::cout << "GetPoints: Expected NULL but got "
              << pspline->GetPoints() << std::endl;
    ++status;
    }

  std::string className;
  className = pspline->GetXSpline()->GetClassName();
  if (className != "vtkCardinalSpline")
    {
    std::cout << "GetXSpline: Expected "
              << "vtkCardinalSpline" << " but got "
              << className << std::endl;
    ++status;
    }
  className = pspline->GetYSpline()->GetClassName();
  if (className != "vtkCardinalSpline")
    {
    std::cout << "GetYSpline: Expected "
              << "vtkCardinalSpline" << " but got "
              << className << std::endl;
    ++status;
    }
  className = pspline->GetZSpline()->GetClassName();
  if (className != "vtkCardinalSpline")
    {
    std::cout << "GetZSpline: Expected "
              << "vtkCardinalSpline" << " but got "
              << className << std::endl;
    ++status;
    }

  // Now change the spline tyeps
  vtkSmartPointer<vtkKochanekSpline> xSpline =
    vtkSmartPointer<vtkKochanekSpline>::New();
  pspline->SetXSpline(xSpline);
  className = pspline->GetXSpline()->GetClassName();
  if (className != "vtkKochanekSpline")
    {
    std::cout << "GetXSpline: Expected "
              << "vtkKochanekSpline" << " but got "
              << className << std::endl;
    ++status;
    }

  vtkSmartPointer<vtkKochanekSpline> ySpline =
    vtkSmartPointer<vtkKochanekSpline>::New();
  pspline->SetYSpline(ySpline);
  className = pspline->GetYSpline()->GetClassName();
  if (className != "vtkKochanekSpline")
    {
    std::cout << "GetYSpline: Expected "
              << "vtkKochanekSpline" << " but got "
              << className << std::endl;
    ++status;
    }

  vtkSmartPointer<vtkKochanekSpline> zSpline =
    vtkSmartPointer<vtkKochanekSpline>::New();
  pspline->SetZSpline(zSpline);
  className = pspline->GetZSpline()->GetClassName();
  if (className != "vtkKochanekSpline")
    {
    std::cout << "GetZSpline: Expected "
              << "vtkKochanekSpline" << " but got "
              << className << std::endl;
    ++status;
    }

  return status;
}

int TestConstraints()
{
  int status = 0;

  vtkSmartPointer<vtkParametricSpline> pspline =
    vtkSmartPointer<vtkParametricSpline>::New();

  pspline->SetNumberOfPoints(2);
  double x[3], result[3];
  x[0] = x[1] = x[2] = 0.0;
  pspline->SetPoint(0, x[0], x[1], x[2]);
  pspline->SetPoint(1, x[0], x[1], x[2]);

  pspline->SetLeftConstraint(2);
  pspline->SetLeftValue(0.0);
  pspline->SetRightConstraint(2);
  pspline->SetRightValue(0.0);

  // Force initialize
  pspline->Evaluate(x, result, NULL);

  if (pspline->GetXSpline()->GetLeftConstraint() !=
      pspline->GetLeftConstraint())
    {
    std::cout << "GetXSpline->GetLeftContraint: Expected "
              << pspline->GetLeftConstraint()
              << " but got " << pspline->GetXSpline()->GetLeftConstraint()
              << std::endl;
    ++status;
    }
  if (pspline->GetXSpline()->GetLeftValue() !=
      pspline->GetLeftValue())
    {
    std::cout << "GetXSpline->GetLeftValue: Expected "
              << pspline->GetLeftValue()
              << " but got " << pspline->GetXSpline()->GetLeftValue()
              << std::endl;
    ++status;
    }

  if (pspline->GetYSpline()->GetLeftConstraint() !=
      pspline->GetLeftConstraint())
    {
    std::cout << "GetYSpline->GetLeftContraint: Expected "
              << pspline->GetLeftConstraint()
              << " but got " << pspline->GetYSpline()->GetLeftConstraint()
              << std::endl;
    ++status;
    }
  if (pspline->GetYSpline()->GetLeftValue() !=
      pspline->GetLeftValue())
    {
    std::cout << "GetYSpline->GetLeftValue: Expected "
              << pspline->GetLeftValue()
              << " but got " << pspline->GetYSpline()->GetLeftValue()
              << std::endl;
    ++status;
    }

  if (pspline->GetZSpline()->GetLeftConstraint() !=
      pspline->GetLeftConstraint())
    {
    std::cout << "GetZSpline->GetLeftContraint: Expected "
              << pspline->GetLeftConstraint()
              << " but got " << pspline->GetZSpline()->GetLeftConstraint()
              << std::endl;
    ++status;
    }
  if (pspline->GetZSpline()->GetLeftValue() !=
      pspline->GetLeftValue())
    {
    std::cout << "GetZSpline->GetLeftValue: Expected "
              << pspline->GetLeftValue()
              << " but got " << pspline->GetZSpline()->GetLeftValue()
              << std::endl;
    ++status;
    }
  return status;
}

int TestMisc()
{
  int status = 0;
  vtkSmartPointer<vtkParametricSpline> pspline =
    vtkSmartPointer<vtkParametricSpline>::New();
  pspline->SetNumberOfPoints(1);
  double x[3];
  x[0] = 1.0;
  x[1] = 0.0;
  x[2] = 0.0;

  if (pspline->EvaluateScalar(x, NULL, NULL) != x[0])
    {
    std::cout << "EvaluateScalar: Expected "
              << x[0]
              << " but got "
              << pspline->EvaluateScalar(x, NULL, NULL)
              << std::endl;
    ++status;
    }
  return status;
}
void TestPrint()
{
  vtkSmartPointer<vtkParametricSpline> pspline =
    vtkSmartPointer<vtkParametricSpline>::New();
  // First test uninitialized spline
  pspline->Print(std::cout);

  // With Points
  double x[3];
  x[0] = x[1] = x[2] = 0.0;

  pspline->SetNumberOfPoints(1);
  pspline->SetPoint(0, x[0], x[1], x[2]);
  pspline->Print(std::cout);

  // With NULL Splines
  pspline->SetXSpline(NULL);
  pspline->SetYSpline(NULL);
  pspline->SetZSpline(NULL);
  pspline->Print(std::cout);
}
