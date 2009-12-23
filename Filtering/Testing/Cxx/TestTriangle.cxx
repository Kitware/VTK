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
#include <vtkstd/limits>
#include "vtkSmartPointer.h"

template<class A>
bool fuzzyCompare(A a, A b) {
  return fabs(a - b) < vtkstd::numeric_limits<A>::epsilon();
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

  return EXIT_SUCCESS;
}
