/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolygon.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests the Polygon

#include "vtkPolygon.h"
#include "vtkPoints.h"
#include <limits>
#include "vtkSmartPointer.h"
#include "vtkIdTypeArray.h"

template<class A>
bool fuzzyCompare(A a, A b) {
  return fabs(a - b) < std::numeric_limits<A>::epsilon();
}

int TestPolygon(int,char *[])
{
  vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New();

  polygon->GetPointIds()->SetNumberOfIds(4);
  polygon->GetPointIds()->SetId(0,0);
  polygon->GetPointIds()->SetId(1,1);
  polygon->GetPointIds()->SetId(2,2);
  polygon->GetPointIds()->SetId(3,3);

  polygon->GetPoints()->SetNumberOfPoints(4);
  polygon->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  polygon->GetPoints()->SetPoint(1, 2.0, 0.0, 0.0);
  polygon->GetPoints()->SetPoint(2, 2.0, 2.0, 0.0);
  polygon->GetPoints()->SetPoint(3, 0.0, 2.0, 0.0);

  double area = polygon->ComputeArea();

  if(!fuzzyCompare(area, 4.0))
    {
    cerr << "ERROR:  polygon area is " << area << ", should be 4.0" << endl;
    return EXIT_FAILURE;
    }

  //////// Test Normal : void vtkPolygon::ComputeNormal (int numPts, double *pts, double n[3]) ///////////
  double normal[3];
  double points[12];
  for(int i = 0; i < polygon->GetNumberOfPoints(); i++)
    {
    double p[3];
    polygon->GetPoints()->GetPoint(i,p);
    points[0 + i*3] = p[0];
    points[1 + i*3] = p[1];
    points[2 + i*3] = p[2];
    }

  vtkPolygon::ComputeNormal(polygon->GetNumberOfPoints(), points, normal);

  if(!fuzzyCompare(normal[0], 0.0) || !fuzzyCompare(normal[1], 0.0) || !fuzzyCompare(normal[2], 1.0))
    {
    cerr << "ERROR: The normal (" << normal[0] << ", " << normal[1] << ", " << normal[2] << " is incorrect (should be (0,0,1))" << endl;
    return EXIT_FAILURE;
    }

  ///////// Test Normal : void vtkPolygon::ComputeNormal(vtkIdTypeArray *ids, vtkPoints *p, double n[3]) /////////
  vtkSmartPointer<vtkIdTypeArray> idArray = vtkSmartPointer<vtkIdTypeArray>::New();
  for(int i = 0; i < polygon->GetNumberOfPoints(); i++)
    {
    idArray->InsertNextValue(i);
    }
  vtkPolygon::ComputeNormal(idArray, polygon->GetPoints(), normal);
  if(!fuzzyCompare(normal[0], 0.0) || !fuzzyCompare(normal[1], 0.0) || !fuzzyCompare(normal[2], 1.0))
    {
    cerr << "ERROR: The normal (" << normal[0] << ", " << normal[1] << ", " << normal[2] << " is incorrect (should be (0,0,1))" << endl;
    return EXIT_FAILURE;
    }

  //////////////////// Polygon intersection test //////////////////
  {
  vtkSmartPointer<vtkPolygon> polygon1 = vtkSmartPointer<vtkPolygon>::New();

  polygon1->GetPointIds()->SetNumberOfIds(4);
  polygon1->GetPointIds()->SetId(0,0);
  polygon1->GetPointIds()->SetId(1,1);
  polygon1->GetPointIds()->SetId(2,2);
  polygon1->GetPointIds()->SetId(3,3);

  polygon1->GetPoints()->SetNumberOfPoints(4);
  polygon1->GetPoints()->SetPoint(0, 0.0, -1.0, -1.0);
  polygon1->GetPoints()->SetPoint(1, 0.0, 1.0, -1.0);
  polygon1->GetPoints()->SetPoint(2, 0.0, 1.0, 1.0);
  polygon1->GetPoints()->SetPoint(3, 0.0, -1.0, 1.0);

  double points1[12];

  for(int i = 0; i < polygon1->GetNumberOfPoints(); i++)
    {
    double p[3];
    polygon1->GetPoints()->GetPoint(i,p);
    points1[0 + i*3] = p[0];
    points1[1 + i*3] = p[1];
    points1[2 + i*3] = p[2];
    }

  double bounds1[6];
  polygon1->GetBounds(bounds1);

  vtkSmartPointer<vtkPolygon> polygon2 = vtkSmartPointer<vtkPolygon>::New();

  polygon2->GetPointIds()->SetNumberOfIds(4);
  polygon2->GetPointIds()->SetId(0,0);
  polygon2->GetPointIds()->SetId(1,1);
  polygon2->GetPointIds()->SetId(2,2);
  polygon2->GetPointIds()->SetId(3,3);

  polygon2->GetPoints()->SetNumberOfPoints(4);
  polygon2->GetPoints()->SetPoint(0, 1.0, -1.0, 0.0);
  polygon2->GetPoints()->SetPoint(1, 1.0, 1.0, 0.0);
  polygon2->GetPoints()->SetPoint(2, -1.0, 1.0, 0.0);
  polygon2->GetPoints()->SetPoint(3, -1.0, -1.0, 0.0);

  double points2[12];
  for(int i = 0; i < polygon2->GetNumberOfPoints(); i++)
    {
    double p[3];
    polygon2->GetPoints()->GetPoint(i,p);
    points2[0 + i*3] = p[0];
    points2[1 + i*3] = p[1];
    points2[2 + i*3] = p[2];
    }

  double bounds2[6];
  polygon2->GetBounds(bounds2);

/*
  int vtkPolygon::IntersectPolygonWithPolygon(int npts, double *pts,double bounds[6],
                                                       int npts2, double *pts2,
                                                       double bounds2[6], double tol2,
                                                       double x[3])
*/
  double intersection[3];
  int intersect = vtkPolygon::IntersectPolygonWithPolygon(static_cast<int>(polygon1->GetNumberOfPoints()), points1, bounds1,
                                                       static_cast<int>(polygon2->GetNumberOfPoints()), points2, bounds2,
                                                       1e-6, intersection);

  if(!intersect)
    {
    return EXIT_FAILURE;
    }

  }

  return EXIT_SUCCESS;
}
