// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests the Triangle

#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTriangle.h"
#include <limits>

int TestTriangle(int, char*[])
{
  // three vertices making a triangle
  double pnt0[3] = { 0, 2, 0 };
  double pnt1[3] = { 4, 2, 0 };
  double pnt2[3] = { 0, 6, 0 };

  // points to be tested against the triangle
  double pnts[][3] = {
    // squared error tolerance
    // = 0.0001 * 0.0001 = 0.00000001

    // outside the triangle
    { 0, 1.999, 0 },
    { -0.001, 2, 0 },

    { 4, 1.999, 0 },
    { 4, 2.001, 0 },
    { 4.001, 2, 0 },

    { 0, 6.001, 0 },
    { 0.001, 6, 0 },
    { -0.001, 6, 0 },

    { -0.001, 2.001, 0 },
    { -0.001, 1.999, 0 },
    { 0.001, 1.999, 0 },

    { 4.001, 2.001, 0 },
    { 4.001, 1.999, 0 },
    { 3.999, 1.999, 0 },

    { -0.001, 5.999, 0 },
    { -0.001, 6.001, 0 },
    { 0.001, 6.001, 0 },

    // inside the triangle
    { 0, 2.001, 0 },
    { 0.001, 2, 0 },
    { 0.001, 2.001, 0 },

    { 3.999, 2.001, 0 },
    { 3.999, 2, 0 },

    { 0, 5.999, 0 },
    { 0.001, 5.999, 0 },

    { 0, 2, 0 },
    { 4, 2, 0 },
    { 0, 6, 0 },

    { 2, 2, 0 },
    { 2, 4, 0 },
    { 0, 4, 0 },
    { 1.333, 3.333, 0 },
  };

  int inside;
  for (int i = 0; i < 31; i++)
  {
    inside = vtkTriangle::PointInTriangle(pnts[i], pnt0, pnt1, pnt2, 0.00000001);

    if (inside && i < 17)
    {
      cerr << "ERROR:  point #" << i
           << ", an outside-point, considered to be inside the triangle!!!" << endl;
      cerr << "Squared error tolerance: 0.00000001" << endl;
      return EXIT_FAILURE;
    }
    else if (!inside && i > 16)
    {
      cerr << "ERROR:  point #" << i
           << ", an inside-point, considered to be outside the triangle!!!" << endl;
      cerr << "Squared error tolerance: 0.00000001" << endl;
      return EXIT_FAILURE;
    }
  }

  cout << "Passed: 17 points outside and 14 points inside the triangle." << endl;

  vtkSmartPointer<vtkTriangle> triangle = vtkSmartPointer<vtkTriangle>::New();
  triangle->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  triangle->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  triangle->GetPoints()->SetPoint(2, 0.0, 1.0, 0.0);

  double area = triangle->ComputeArea();
  if (!vtkMathUtilities::NearlyEqual<double>(area, 0.5))
  {
    cerr << "ERROR:  triangle area is " << area << ", should be 0.5" << endl;
    return EXIT_FAILURE;
  }

  // Testing degenerated triangle
  double pntDeg0[3] = { 0, 0, -10 };
  double pntDeg1[3] = { 0, 0, 0 };
  double pntDeg2[3] = { 0, 0, 10 };
  vtkNew<vtkTriangle> triangleDeg;
  triangleDeg->GetPoints()->SetPoint(0, pntDeg0);
  triangleDeg->GetPoints()->SetPoint(1, pntDeg1);
  triangleDeg->GetPoints()->SetPoint(2, pntDeg2);

  double p1[3] = { 0, 1, 1 };
  double p2[3] = { 0, -1, 1 };
  double t;
  double x[3];
  double pcoords[3];
  int subId;
  double dEpsilon = std::numeric_limits<double>::epsilon();
  if (triangleDeg->IntersectWithLine(p1, p2, dEpsilon, t, x, pcoords, subId) != 1 ||
    !vtkMathUtilities::NearlyEqual<double>(x[0], 0.0) ||
    !vtkMathUtilities::NearlyEqual<double>(x[1], 0.0) ||
    !vtkMathUtilities::NearlyEqual<double>(x[2], 1.0) ||
    !vtkMathUtilities::NearlyEqual<double>(t, 0.5) ||
    !vtkMathUtilities::NearlyEqual<double>(pcoords[0], 1.1) ||
    !vtkMathUtilities::NearlyEqual<double>(pcoords[1], 0.55) ||
    !vtkMathUtilities::NearlyEqual<double>(pcoords[2], 0.0))
  {
    cerr << "Error while intersecting degenerated triangle" << endl;
    return EXIT_FAILURE;
  }
  double p1b[3] = { 0, 1, 10.001 };
  double p2b[3] = { 0, -1, 10.001 };
  if (triangleDeg->IntersectWithLine(p1b, p2b, dEpsilon, t, x, pcoords, subId) != 0)
  {
    cerr << "Error while intersecting degenerated triangle" << endl;
    return EXIT_FAILURE;
  }

  // Testing intersection of triangle with coplanar line

  // Build triangle
  double pt0[3] = { 0, 0, 0 };
  double pt1[3] = { 0, 10, 0 };
  double pt2[3] = { 0, 0, 10 };
  vtkNew<vtkTriangle> coplanarTriangle;
  coplanarTriangle->GetPoints()->SetPoint(0, pt0);
  coplanarTriangle->GetPoints()->SetPoint(1, pt1);
  coplanarTriangle->GetPoints()->SetPoint(2, pt2);

  // Define line extremities with first extremity inside
  double ext1[3] = { 0, 1, 5 };
  double ext2[3] = { 0, 11, 5 };

  int res = coplanarTriangle->IntersectWithLine(ext1, ext2, dEpsilon, t, x, pcoords, subId);
  // Verify correct output values
  if (res != 1)
  {
    cerr << "Line intersection with coplanar triangle not detected" << endl;
    return EXIT_FAILURE;
  }
  else if (x[0] != 0 || x[1] != 1 || x[2] != 5 || t != 0.0 || pcoords[0] != 0.1 ||
    pcoords[1] != 0.5 || pcoords[2] != 0.0)
  {
    cerr << "Output coordinates of intersecting point incorrect" << endl;
    return EXIT_FAILURE;
  }

  // Define line extremities with first extremity outside
  ext1[0] = 0;
  ext1[1] = -1;
  ext1[2] = 5;
  ext2[0] = 0;
  ext2[1] = 9;
  ext2[2] = 5;

  res = coplanarTriangle->IntersectWithLine(ext1, ext2, dEpsilon, t, x, pcoords, subId);
  // Verify correct output values
  if (res != 1)
  {
    cerr << "Line intersection with coplanar triangle not detected" << endl;
    return EXIT_FAILURE;
  }
  else if (x[0] != 0 || x[1] != 0 || x[2] != 5 || t != 0.1 || pcoords[0] != 0.0 ||
    pcoords[1] != 0.5 || pcoords[2] != 0.0)
  {
    cerr << "Output coordinates of intersecting point incorrect" << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
