// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests the Triangle

#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkTriangle.h"

#include <algorithm>
#include <limits>

#include <iostream>

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

  for (int i = 0; i < 31; i++)
  {
    int inside = vtkTriangle::PointInTriangle(pnts[i], pnt0, pnt1, pnt2, 0.00000001);

    if (inside && i < 17)
    {
      std::cerr << "ERROR:  point #" << i
                << ", an outside-point, considered to be inside the triangle!!!" << std::endl;
      std::cerr << "Squared error tolerance: 0.00000001" << std::endl;
      return EXIT_FAILURE;
    }
    else if (!inside && i > 16)
    {
      std::cerr << "ERROR:  point #" << i
                << ", an inside-point, considered to be outside the triangle!!!" << std::endl;
      std::cerr << "Squared error tolerance: 0.00000001" << std::endl;
      return EXIT_FAILURE;
    }
  }

  std::cout << "Passed: 17 points outside and 14 points inside the triangle." << std::endl;

  vtkSmartPointer<vtkTriangle> triangle = vtkSmartPointer<vtkTriangle>::New();
  triangle->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  triangle->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  triangle->GetPoints()->SetPoint(2, 0.0, 1.0, 0.0);

  double area = triangle->ComputeArea();
  if (!vtkMathUtilities::NearlyEqual<double>(area, 0.5))
  {
    std::cerr << "ERROR:  triangle area is " << area << ", should be 0.5" << std::endl;
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
    std::cerr << "Error while intersecting degenerated triangle" << std::endl;
    return EXIT_FAILURE;
  }
  double p1b[3] = { 0, 1, 10.001 };
  double p2b[3] = { 0, -1, 10.001 };
  if (triangleDeg->IntersectWithLine(p1b, p2b, dEpsilon, t, x, pcoords, subId) != 0)
  {
    std::cerr << "Error while intersecting degenerated triangle" << std::endl;
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
    std::cerr << "Line intersection with coplanar triangle not detected" << std::endl;
    return EXIT_FAILURE;
  }
  else if (x[0] != 0 || x[1] != 1 || x[2] != 5 || t != 0.0 || pcoords[0] != 0.1 ||
    pcoords[1] != 0.5 || pcoords[2] != 0.0)
  {
    std::cerr << "Output coordinates of intersecting point incorrect" << std::endl;
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
    std::cerr << "Line intersection with coplanar triangle not detected" << std::endl;
    return EXIT_FAILURE;
  }
  else if (x[0] != 0 || x[1] != 0 || x[2] != 5 || t != 0.1 || pcoords[0] != 0.0 ||
    pcoords[1] != 0.5 || pcoords[2] != 0.0)
  {
    std::cerr << "Output coordinates of intersecting point incorrect" << std::endl;
    return EXIT_FAILURE;
  }

  // Testing vtkTriangle::EvaluatePosition with points in different relative configurations

  // Points defining the vertices of an obtuse triangle not aligned with coordinate axes.
  double ptB0[3] = { 1, 0, 0 };
  double ptB1[3] = { 2, 0, 0 };
  double ptB2[3] = { 0, 1, 1 };

  // Points to test the relative position and the closest point
  struct TestPointData
  {
    double p[3];
    double closestP[3];
    int inside;
  };
  std::vector<TestPointData> data = {
    // Points that are already inside the triangle (strictly coplanar)
    { { 1.00001, 0.00001, 0.00001 }, { 1.00001, 0.00001, 0.00001 }, 1 },
    { { 1.00001, 0.49999, 0.49999 }, { 1.00001, 0.49999, 0.49999 }, 1 },
    { { 1.99996, 0.00001, 0.00001 }, { 1.99996, 0.00001, 0.00001 }, 1 },
    { { 0.5, 0.50001, 0.50001 }, { 0.5, 0.50001, 0.50001 }, 1 },
    { { 0.5, 0.625, 0.625 }, { 0.5, 0.625, 0.625 }, 1 },
    { { 0.5, 0.74999, 0.74999 }, { 0.5, 0.74999, 0.74999 }, 1 },
    { { 0.00004, 0.99997, 0.99997 }, { 0.00004, 0.99997, 0.99997 }, 1 },
    // Inside the triangle bounding box and with projection inside the triangle.
    // This is currently considered inside (bug or feature?)
    { { 1.00001, 0.00001, 0.99997 }, { 1.00001, 0.49999, 0.49999 }, 1 },
    { { 1.00001, 0.99997, 0.00001 }, { 1.00001, 0.49999, 0.49999 }, 1 },
    { { 0.5, 0.00003, 0.99999 }, { 0.5, 0.50001, 0.50001 }, 1 },
    { { 0.5, 0.99999, 0.00003 }, { 0.5, 0.50001, 0.50001 }, 1 },
    { { 0.5, 0.25001, 0.99999 }, { 0.5, 0.625, 0.625 }, 1 },
    { { 0.5, 0.99999, 0.25001 }, { 0.5, 0.625, 0.625 }, 1 },
    // Outside the triangle, projecting to edge 0-1
    { { 1.00001, -0.5, -0.5 }, { 1.00001, 0., 0. }, 0 },
    { { 1.00001, -0.5, 0 }, { 1.00001, 0., 0. }, 0 },
    { { 1.99999, -0.5, -0.5 }, { 1.99999, 0., 0. }, 0 },
    { { 1.99999, 0, -0.5 }, { 1.99999, 0., 0. }, 0 },
    // Outside the triangle, projecting to edge 1-2
    { { 2.99998, 1.00001, 1.00001 }, { 1.99998, 0.00001, 0.00001 }, 0 },
    { { 2.99998, 0.00001, 2.00001 }, { 1.99998, 0.00001, 0.00001 }, 0 },
    { { 1.00002, 1.99999, 1.99999 }, { 0.00002, 0.99999, 0.99999 }, 0 },
    { { 1.00002, 2.99999, 0.99999 }, { 0.00002, 0.99999, 0.99999 }, 0 },
    // Outside the triangle, projecting to edge 2-0
    { { -0.00001, 0.99998, 0.99998 }, { 0.00001, 0.99999, 0.99999 }, 0 },
    { { -0.00001, -0.00002, 1.99998 }, { 0.00001, 0.99999, 0.99999 }, 0 },
    { { -0.00001, -0.49999, -0.49999 }, { 0.99999, 0.00001, 0.00001 }, 0 },
    { { -0.00001, 0.00001, -0.99999 }, { 0.99999, 0.00001, 0.00001 }, 0 },
    // Outside the triangle, projecting to vertex 0
    { { 0.9, -0.5, -0.5 }, { 1., 0., 0. }, 0 },
    { { 0.9, -1., 0. }, { 1., 0., 0. }, 0 },
    { { 0.1, -0.5, -0.5 }, { 1., 0., 0. }, 0 },
    { { 0.1, 0., -1. }, { 1., 0., 0. }, 0 },
    // Outside the triangle, projecting to vertex 1
    { { 2.1, -0.5, -0.5 }, { 2., 0., 0. }, 0 },
    { { 2.1, 0.5, -1.5 }, { 2., 0., 0. }, 0 },
    { { 3., 0.9, 0.9 }, { 2., 0., 0. }, 0 },
    { { 3., 1.4, 0.4 }, { 2., 0., 0. }, 0 },
    // Outside the triangle, projecting to vertex 2
    { { 0.9, 2., 2. }, { 0., 1., 1. }, 0 },
    { { 0.9, 1., 3. }, { 0., 1., 1. }, 0 },
    { { -0.3, 0.9, 0.9 }, { 0., 1., 1. }, 0 },
    { { -0.3, 0.4, 1.4 }, { 0., 1., 1. }, 0 },
  };

  std::vector<int> pId = { 0, 1, 2 };
  do
  {
    vtkNew<vtkTriangle> obtuseTriangle;
    vtkPoints* pts = obtuseTriangle->GetPoints();
    pts->SetPoint(pId[0], ptB0);
    pts->SetPoint(pId[1], ptB1);
    pts->SetPoint(pId[2], ptB2);

    double closestPoint[3];
    double weights[3];
    double dist2;
    for (unsigned int i = 0; i < data.size(); ++i)
    {
      int inside =
        obtuseTriangle->EvaluatePosition(data[i].p, closestPoint, subId, pcoords, dist2, weights);
      if (inside != data[i].inside)
      {
        std::cerr << "ERROR: EvaluatePosition on point " << vtk::format("{}", data[i].p)
                  << std::endl
                  << "       and vertex permutation " << vtk::format("{}", pId) << std::endl
                  << "       returns value " << inside << std::endl
                  << "       not coinciding with insideGT = " << data[i].inside << std::endl;
        ;
        return EXIT_FAILURE;
      }
      if (vtkMath::Distance2BetweenPoints(closestPoint, data[i].closestP) > 1e-12)
      {
        std::cerr << "ERROR: EvaluatePosition on point " << vtk::format("{}", data[i].p)
                  << std::endl
                  << "       and vertex permutation " << vtk::format("{}", pId) << std::endl
                  << "       gives the closest point " << vtk::format("{}", closestPoint)
                  << std::endl
                  << "       when it should be " << vtk::format("{}", data[i].closestP)
                  << std::endl;
        return EXIT_FAILURE;
      }
    }
  } while (std::next_permutation(pId.begin(), pId.end()));

  return EXIT_SUCCESS;
}
