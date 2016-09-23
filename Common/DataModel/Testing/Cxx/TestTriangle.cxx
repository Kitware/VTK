/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTriangle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests the Triangle

#include "vtkTriangle.h"
#include "vtkPoints.h"
#include <limits>
#include "vtkSmartPointer.h"
#include "vtkNew.h"

template<class A>
bool fuzzyCompare(A a, A b) {
  return fabs(a - b) < std::numeric_limits<A>::epsilon();
}

int TestTriangle(int,char *[])
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
                       {  0,     1.999, 0 },
                       { -0.001, 2,     0 },

                       {  4,     1.999, 0 },
                       {  4,     2.001, 0 },
                       {  4.001, 2,     0 },

                       {  0,     6.001, 0 },
                       {  0.001, 6,     0 },
                       { -0.001, 6,     0 },

                       { -0.001, 2.001, 0 },
                       { -0.001, 1.999, 0 },
                       {  0.001, 1.999, 0 },

                       {  4.001, 2.001, 0 },
                       {  4.001, 1.999, 0 },
                       {  3.999, 1.999, 0 },

                       { -0.001, 5.999, 0 },
                       { -0.001, 6.001, 0 },
                       {  0.001, 6.001, 0 },

                       // inside the triangle
                       {  0,     2.001, 0 },
                       {  0.001, 2,     0 },
                       {  0.001, 2.001, 0 },

                       {  3.999, 2.001, 0 },
                       {  3.999, 2,     0 },

                       {  0,     5.999, 0 },
                       {  0.001, 5.999, 0 },

                       {  0,     2,     0 },
                       {  4,     2,     0 },
                       {  0,     6,     0 },

                       {  2,     2,     0 },
                       {  2,     4,     0 },
                       {  0,     4,     0 },
                       {  1.333, 3.333, 0 }
                     };

  int inside;
  for ( int i = 0; i < 31; i ++ )
  {
    inside = vtkTriangle::PointInTriangle( pnts[i], pnt0, pnt1, pnt2, 0.00000001 );

    if ( inside && i < 17 )
    {
      cerr << "ERROR:  point #" << i << ", an outside-point, considered to be inside the triangle!!!" << endl;
      cerr << "Squared error tolerance: 0.00000001" << endl;
      return EXIT_FAILURE;
    }
    else
    if ( !inside && i > 16 )
    {
      cerr << "ERROR:  point #" << i << ", an inside-point, considered to be outside the triangle!!!" << endl;
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
  if(!fuzzyCompare(area, 0.5))
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
  if (triangleDeg->IntersectWithLine(p1, p2, dEpsilon, t, x, pcoords, subId) != 1
     || x[0] != 0 || x[1] != 0 || x[2] != 1 || t != 0.5 || pcoords[0] != 1.1
     || pcoords[1] != 0.55 || pcoords[2] != 0)
  {
    cerr<<"Error while intersecting degenerated triangle"<<endl;
    return EXIT_FAILURE;
  }
  double p1b[3] = { 0, 1, 10.001 };
  double p2b[3] = { 0, -1, 10.001 };
  if (triangleDeg->IntersectWithLine(p1b, p2b, dEpsilon, t, x, pcoords, subId) != 0)
  {
    cerr<<"Error while intersecting degenerated triangle"<<endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
