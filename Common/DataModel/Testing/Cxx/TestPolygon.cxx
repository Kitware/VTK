// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests the Polygon

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMathUtilities.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include <limits>

#include <iostream>

int TestPolygon(int, char*[])
{
  vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New();

  polygon->GetPointIds()->SetNumberOfIds(4);
  polygon->GetPointIds()->SetId(0, 0);
  polygon->GetPointIds()->SetId(1, 1);
  polygon->GetPointIds()->SetId(2, 2);
  polygon->GetPointIds()->SetId(3, 3);

  polygon->GetPoints()->SetNumberOfPoints(4);
  polygon->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  polygon->GetPoints()->SetPoint(1, 2.0, 0.0, 0.0);
  polygon->GetPoints()->SetPoint(2, 2.0, 2.0, 0.0);
  polygon->GetPoints()->SetPoint(3, 0.0, 2.0, 0.0);

  double area = polygon->ComputeArea();

  if (!vtkMathUtilities::NearlyEqual<double>(area, 4.0))
  {
    std::cerr << "ERROR:  polygon area is " << area << ", should be 4.0" << std::endl;
    return EXIT_FAILURE;
  }

  // Test convexity. First, ensure the square is convex.
  {
    vtkIdType idTypeArray[4] = { 0, 1, 2, 3 };

    vtkSmartPointer<vtkIdTypeArray> idArray = vtkSmartPointer<vtkIdTypeArray>::New();
    for (int i = 0; i < polygon->GetNumberOfPoints(); i++)
    {
      idArray->InsertNextValue(i);
    }

    bool convex;
    convex = polygon->IsConvex();
    convex &= vtkPolygon::IsConvex(polygon->GetPoints(), polygon->GetNumberOfPoints(), idTypeArray);
    convex &= vtkPolygon::IsConvex(idArray, polygon->GetPoints());
    convex &= vtkPolygon::IsConvex(polygon->GetPoints());

    if (!convex)
    {
      std::cerr << "ERROR:  polygon should be classified as convex" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Next, create a nonconvex element and test it.
  {
    polygon->GetPoints()->SetPoint(3, 1.5, 0.5, 0.0);

    vtkIdType idTypeArray[4] = { 0, 1, 2, 3 };

    vtkSmartPointer<vtkIdTypeArray> idArray = vtkSmartPointer<vtkIdTypeArray>::New();
    for (int i = 0; i < polygon->GetNumberOfPoints(); i++)
    {
      idArray->InsertNextValue(i);
    }

    bool nonconvex;
    nonconvex = !polygon->IsConvex();
    nonconvex &=
      !vtkPolygon::IsConvex(polygon->GetPoints(), polygon->GetNumberOfPoints(), idTypeArray);
    nonconvex &= !vtkPolygon::IsConvex(idArray, polygon->GetPoints());
    nonconvex &= !vtkPolygon::IsConvex(polygon->GetPoints());

    if (!nonconvex)
    {
      std::cerr << "ERROR:  polygon should be classified as nonconvex" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Next, create an element with a colinear point and test it.
  {
    polygon->GetPoints()->SetPoint(3, 1.0, 1.0, 0.0);

    vtkIdType idTypeArray[4] = { 0, 1, 2, 3 };

    vtkSmartPointer<vtkIdTypeArray> idArray = vtkSmartPointer<vtkIdTypeArray>::New();
    for (int i = 0; i < polygon->GetNumberOfPoints(); i++)
    {
      idArray->InsertNextValue(i);
    }

    bool convex;
    convex = polygon->IsConvex();
    convex &= vtkPolygon::IsConvex(polygon->GetPoints(), polygon->GetNumberOfPoints(), idTypeArray);
    convex &= vtkPolygon::IsConvex(idArray, polygon->GetPoints());
    convex &= vtkPolygon::IsConvex(polygon->GetPoints());

    if (!convex)
    {
      std::cerr << "ERROR:  polygon should be classified as convex" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // Finally, create an element with a degenerate point and test it.
  {
    polygon->GetPoints()->SetPoint(3, 2.0, 2.0, 0.0);

    vtkIdType idTypeArray[4] = { 0, 1, 2, 3 };

    vtkSmartPointer<vtkIdTypeArray> idArray = vtkSmartPointer<vtkIdTypeArray>::New();
    for (int i = 0; i < polygon->GetNumberOfPoints(); i++)
    {
      idArray->InsertNextValue(i);
    }

    bool convex;
    convex = polygon->IsConvex();
    convex &= vtkPolygon::IsConvex(polygon->GetPoints(), polygon->GetNumberOfPoints(), idTypeArray);
    convex &= vtkPolygon::IsConvex(idArray, polygon->GetPoints());
    convex &= vtkPolygon::IsConvex(polygon->GetPoints());

    if (!convex)
    {
      std::cerr << "ERROR:  polygon should be classified as convex" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // return the element to its original state.
  polygon->GetPoints()->SetPoint(3, 0.0, 2.0, 0.0);

  // Test Normal : void vtkPolygon::ComputeNormal (int numPts, double *pts, double n[3])
  double normal[3];
  double points[12];
  for (int i = 0; i < polygon->GetNumberOfPoints(); i++)
  {
    double p[3];
    polygon->GetPoints()->GetPoint(i, p);
    points[0 + i * 3] = p[0];
    points[1 + i * 3] = p[1];
    points[2 + i * 3] = p[2];
  }

  vtkPolygon::ComputeNormal(polygon->GetNumberOfPoints(), points, normal);

  if (!vtkMathUtilities::NearlyEqual<double>(normal[0], 0.0) ||
    !vtkMathUtilities::NearlyEqual<double>(normal[1], 0.0) ||
    !vtkMathUtilities::NearlyEqual<double>(normal[2], 1.0))
  {
    std::cerr << "ERROR: The normal (" << normal[0] << ", " << normal[1] << ", " << normal[2]
              << " is incorrect (should be (0,0,1))" << std::endl;
    return EXIT_FAILURE;
  }

  // Test Normal : void vtkPolygon::ComputeNormal(vtkIdTypeArray *ids, vtkPoints *p, double n[3])
  vtkSmartPointer<vtkIdTypeArray> idArray = vtkSmartPointer<vtkIdTypeArray>::New();
  for (int i = 0; i < polygon->GetNumberOfPoints(); i++)
  {
    idArray->InsertNextValue(i);
  }
  vtkPolygon::ComputeNormal(idArray, polygon->GetPoints(), normal);
  if (!vtkMathUtilities::NearlyEqual<double>(normal[0], 0.0) ||
    !vtkMathUtilities::NearlyEqual<double>(normal[1], 0.0) ||
    !vtkMathUtilities::NearlyEqual<double>(normal[2], 1.0))
  {
    std::cerr << "ERROR: The normal (" << normal[0] << ", " << normal[1] << ", " << normal[2]
              << " is incorrect (should be (0,0,1))" << std::endl;
    return EXIT_FAILURE;
  }

  // Polygon intersection test
  {
    vtkSmartPointer<vtkPolygon> polygon1 = vtkSmartPointer<vtkPolygon>::New();

    polygon1->GetPointIds()->SetNumberOfIds(4);
    polygon1->GetPointIds()->SetId(0, 0);
    polygon1->GetPointIds()->SetId(1, 1);
    polygon1->GetPointIds()->SetId(2, 2);
    polygon1->GetPointIds()->SetId(3, 3);

    polygon1->GetPoints()->SetNumberOfPoints(4);
    polygon1->GetPoints()->SetPoint(0, 0.0, -1.0, -1.0);
    polygon1->GetPoints()->SetPoint(1, 0.0, 1.0, -1.0);
    polygon1->GetPoints()->SetPoint(2, 0.0, 1.0, 1.0);
    polygon1->GetPoints()->SetPoint(3, 0.0, -1.0, 1.0);

    double points1[12];

    for (int i = 0; i < polygon1->GetNumberOfPoints(); i++)
    {
      double p[3];
      polygon1->GetPoints()->GetPoint(i, p);
      points1[0 + i * 3] = p[0];
      points1[1 + i * 3] = p[1];
      points1[2 + i * 3] = p[2];
    }

    double bounds1[6];
    polygon1->GetBounds(bounds1);

    vtkSmartPointer<vtkPolygon> polygon2 = vtkSmartPointer<vtkPolygon>::New();

    polygon2->GetPointIds()->SetNumberOfIds(4);
    polygon2->GetPointIds()->SetId(0, 0);
    polygon2->GetPointIds()->SetId(1, 1);
    polygon2->GetPointIds()->SetId(2, 2);
    polygon2->GetPointIds()->SetId(3, 3);

    polygon2->GetPoints()->SetNumberOfPoints(4);
    polygon2->GetPoints()->SetPoint(0, 1.0, -1.0, 0.0);
    polygon2->GetPoints()->SetPoint(1, 1.0, 1.0, 0.0);
    polygon2->GetPoints()->SetPoint(2, -1.0, 1.0, 0.0);
    polygon2->GetPoints()->SetPoint(3, -1.0, -1.0, 0.0);

    double points2[12];
    for (int i = 0; i < polygon2->GetNumberOfPoints(); i++)
    {
      double p[3];
      polygon2->GetPoints()->GetPoint(i, p);
      points2[0 + i * 3] = p[0];
      points2[1 + i * 3] = p[1];
      points2[2 + i * 3] = p[2];
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
    int intersect = vtkPolygon::IntersectPolygonWithPolygon(
      static_cast<int>(polygon1->GetNumberOfPoints()), points1, bounds1,
      static_cast<int>(polygon2->GetNumberOfPoints()), points2, bounds2, 1e-6, intersection);

    if (!intersect)
    {
      return EXIT_FAILURE;
    }
  }

  // Test that clipping a polygon produces a single output polygon (not a set
  // of triangles), and that point data at the newly-created cut points is
  // interpolated correctly.
  {
    // A 5-sided "house" polygon: a unit-height square base with a triangular
    // roof whose apex reaches y = 2.
    vtkSmartPointer<vtkPolygon> house = vtkSmartPointer<vtkPolygon>::New();
    house->GetPointIds()->SetNumberOfIds(5);
    house->GetPoints()->SetNumberOfPoints(5);
    const double houseCoords[5][3] = {
      { 0.0, 0.0, 0.0 },
      { 2.0, 0.0, 0.0 },
      { 2.0, 1.0, 0.0 },
      { 1.0, 2.0, 0.0 }, // apex
      { 0.0, 1.0, 0.0 },
    };
    for (vtkIdType i = 0; i < 5; i++)
    {
      house->GetPointIds()->SetId(i, i);
      house->GetPoints()->SetPoint(i, houseCoords[i]);
    }

    // Clip using the y-coordinate as the scalar field, cutting through the
    // roof above the eaves (edges (2,3) and (3,4)) but below the apex.
    const double clipValue = 1.5;
    vtkSmartPointer<vtkDoubleArray> cellScalars = vtkSmartPointer<vtkDoubleArray>::New();
    cellScalars->SetNumberOfTuples(5);
    for (vtkIdType i = 0; i < 5; i++)
    {
      cellScalars->SetValue(i, houseCoords[i][1]);
    }

    // Point data to be interpolated at the cut points, distinct from the
    // clip scalar field so that interpolation is exercised independently.
    vtkSmartPointer<vtkDoubleArray> field = vtkSmartPointer<vtkDoubleArray>::New();
    field->SetName("field");
    field->SetNumberOfTuples(5);
    for (vtkIdType i = 0; i < 5; i++)
    {
      field->SetValue(i, 10.0 * i);
    }
    vtkSmartPointer<vtkPointData> inPD = vtkSmartPointer<vtkPointData>::New();
    inPD->AddArray(field);
    vtkSmartPointer<vtkPointData> outPD = vtkSmartPointer<vtkPointData>::New();
    outPD->InterpolateAllocate(inPD);

    vtkSmartPointer<vtkCellData> inCD = vtkSmartPointer<vtkCellData>::New();
    vtkSmartPointer<vtkCellData> outCD = vtkSmartPointer<vtkCellData>::New();
    outCD->CopyAllocate(inCD);

    vtkSmartPointer<vtkPoints> outPoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkPointLocator> locator = vtkSmartPointer<vtkPointLocator>::New();
    double bounds[6] = { -1.0, 3.0, -1.0, 3.0, -1.0, 1.0 };
    locator->InitPointInsertion(outPoints, bounds);

    vtkSmartPointer<vtkCellArray> outPolys = vtkSmartPointer<vtkCellArray>::New();

    // insideOut = 1 keeps the low (base) side of the clip plane, i.e. the
    // hexagon formed by the square base plus the lower portion of the roof.
    house->Clip(clipValue, cellScalars, locator, outPolys, inPD, outPD, inCD, 0, outCD, 1);

    if (outPolys->GetNumberOfCells() != 1)
    {
      std::cerr << "ERROR: clipping the house polygon should produce exactly one polygon, got "
                << outPolys->GetNumberOfCells() << std::endl;
      return EXIT_FAILURE;
    }

    vtkIdType npts;
    const vtkIdType* pts;
    outPolys->GetCellAtId(0, npts, pts);

    if (npts != 6)
    {
      std::cerr << "ERROR: the clipped house polygon should have 6 points, got " << npts
                << std::endl;
      return EXIT_FAILURE;
    }

    // Expected points, in order, with the point-data field value expected at
    // each. The two new points cut the roof edges (2,3) and (3,4) exactly
    // halfway (since the apex sits at y = 2, the eaves at y = 1, and the
    // clip value is 1.5), so their field values are the midpoint of the
    // field values at the edge's endpoints.
    const double expectedXYZ[6][3] = {
      { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 2.0, 0.0, 0.0 }, { 2.0, 1.0, 0.0 },
      { 1.5, 1.5, 0.0 }, // cut point on edge (2,3)
      { 0.5, 1.5, 0.0 }, // cut point on edge (3,4)
    };
    const double expectedField[6] = { 40.0, 0.0, 10.0, 20.0, 25.0, 35.0 };

    vtkDoubleArray* outField = vtkDoubleArray::SafeDownCast(outPD->GetArray("field"));
    if (!outField)
    {
      std::cerr << "ERROR: the clipped polygon's output point data is missing the 'field' array"
                << std::endl;
      return EXIT_FAILURE;
    }

    for (vtkIdType i = 0; i < npts; i++)
    {
      double p[3];
      outPoints->GetPoint(pts[i], p);
      if (!vtkMathUtilities::NearlyEqual<double>(p[0], expectedXYZ[i][0]) ||
        !vtkMathUtilities::NearlyEqual<double>(p[1], expectedXYZ[i][1]) ||
        !vtkMathUtilities::NearlyEqual<double>(p[2], expectedXYZ[i][2]))
      {
        std::cerr << "ERROR: clipped house polygon point " << i << " is (" << p[0] << ", " << p[1]
                  << ", " << p[2] << "), expected (" << expectedXYZ[i][0] << ", "
                  << expectedXYZ[i][1] << ", " << expectedXYZ[i][2] << ")" << std::endl;
        return EXIT_FAILURE;
      }

      const double fieldValue = outField->GetValue(pts[i]);
      if (!vtkMathUtilities::NearlyEqual<double>(fieldValue, expectedField[i]))
      {
        std::cerr << "ERROR: clipped house polygon point " << i << " has field value " << fieldValue
                  << ", expected " << expectedField[i] << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
