/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyhedronConvexity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vector>

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkExtractEdges.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyhedron.h"
#include "vtkProperty.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

enum Shape
{
  dodecahedron,
  u_shape,
  cube,
  colinear_cube,
  degenerate_cube,
  convex_pyramid,
  nonconvex_pyramid
};

bool IsConvex(Shape shape)
{
  vtkSmartPointer<vtkPoints> polyhedronPoints =
    vtkSmartPointer<vtkPoints>::New();

  vtkSmartPointer<vtkCellArray> polyhedronFaces =
    vtkSmartPointer<vtkCellArray>::New();

  std::vector<vtkIdType> polyhedronPointsIds;

  if (shape == dodecahedron)
  {
    // create a dodecahedron
    const int nPoints = 20;
    const int nFaces = 12;

    double dodechedronPoint[nPoints][3] = { {1.21412,    0,          1.58931},
                                            {0.375185,   1.1547,     1.58931},
                                            {-0.982247,  0.713644,   1.58931},
                                            {-0.982247,  -0.713644,  1.58931},
                                            {0.375185,   -1.1547,    1.58931},
                                            {1.96449,    0,          0.375185},
                                            {0.607062,   1.86835,    0.375185},
                                            {-1.58931,   1.1547,     0.375185},
                                            {-1.58931,   -1.1547,    0.375185},
                                            {0.607062,   -1.86835,   0.375185},
                                            {1.58931,    1.1547,     -0.375185},
                                            {-0.607062,  1.86835,    -0.375185},
                                            {-1.96449,   0,          -0.375185},
                                            {-0.607062,  -1.86835,   -0.375185},
                                            {1.58931,    -1.1547,    -0.375185},
                                            {0.982247,   0.713644,   -1.58931},
                                            {-0.375185,  1.1547,     -1.58931},
                                            {-1.21412,   0,          -1.58931},
                                            {-0.375185,  -1.1547,    -1.58931},
                                            {0.982247,   -0.713644,  -1.58931}};
    polyhedronPoints->SetNumberOfPoints(nPoints);
    for (int i = 0; i < nPoints; i++)
    {
        polyhedronPoints->SetPoint(i,dodechedronPoint[i]);
        polyhedronPointsIds.push_back(i);
    }

    vtkIdType dodechedronFace[nFaces][5] = {{0, 1, 2, 3, 4},
                                            {0, 5, 10, 6, 1},
                                            {1, 6, 11, 7, 2},
                                            {2, 7, 12, 8, 3},
                                            {3, 8, 13, 9, 4},
                                            {4, 9, 14, 5, 0},
                                            {15, 10, 5, 14, 19},
                                            {16, 11, 6, 10, 15},
                                            {17, 12, 7, 11, 16},
                                            {18, 13, 8, 12, 17},
                                            {19, 14, 9, 13, 18},
                                            {19, 18, 17, 16, 15}};

    for (int i = 0; i < nFaces; i++)
    {
      polyhedronFaces->InsertNextCell(5, dodechedronFace[i]);
    }
  }
  else if (shape == u_shape)
  {
    // create a concave shape
    const int nPoints = 16;
    const int nFaces = 10;

    double concaveShapePoint[nPoints][3] = {{.5,   -.5,  .25},
                                            {.5,   .5,   .25},
                                            {.25,  .5,   .25},
                                            {.25,  -.25, .25},
                                            {-.25, -.25, .25},
                                            {-.25, .5,   .25},
                                            {-.5,  .5,   .25},
                                            {-.5,  -.5,  .25},
                                            {.5,   -.5,  -.25},
                                            {.5,   .5,   -.25},
                                            {.25,  .5,   -.25},
                                            {.25,  -.25, -.25},
                                            {-.25, -.25, -.25},
                                            {-.25, .5,   -.25},
                                            {-.5,  .5,   -.25},
                                            {-.5,  -.5,  -.25}};

    polyhedronPoints->SetNumberOfPoints(nPoints);
    for (int i = 0; i < nPoints; i++)
    {
      polyhedronPoints->SetPoint(i,concaveShapePoint[i]);
      polyhedronPointsIds.push_back(i);
    }

    vtkIdType f1[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    vtkIdType f2[8] = {15, 14, 13, 12, 11, 10, 9, 8};
    vtkIdType f3[4] = {0, 7, 15, 8};
    vtkIdType f4[4] = {1, 0, 8, 9};
    vtkIdType f5[4] = {2, 1, 9, 10};
    vtkIdType f6[4] = {3, 2, 10, 11};
    vtkIdType f7[4] = {4, 3, 11, 12};
    vtkIdType f8[4] = {5, 4, 12, 13};
    vtkIdType f9[4] = {6, 5, 13, 14};
    vtkIdType f10[4] = {7, 6, 14, 15};

    vtkIdType* concaveShapeFace[nFaces] = {f1,f2,f3,f4,f5,f6,f7,f8,f9,f10};

    for (int i = 0; i < nFaces; i++)
    {
      polyhedronFaces->InsertNextCell((i >= 2 ? 4 : 8),
                                      concaveShapeFace[i]);
    }
  }
  else if (shape == cube)
  {
    // create a cube
    const int nPoints = 8;
    const int nFaces = 6;

    double cubeShapePoint[nPoints][3] = {{.5,   .5,  .5},
                                         {-.5,  .5,  .5},
                                         {-.5, -.5,  .5},
                                         {.5,  -.5,  .5},
                                         {.5,   .5, -.5},
                                         {-.5,  .5, -.5},
                                         {-.5, -.5, -.5},
                                         {.5,  -.5, -.5}};

    polyhedronPoints->SetNumberOfPoints(nPoints);
    for (int i = 0; i < nPoints; i++)
    {
      polyhedronPoints->SetPoint(i,cubeShapePoint[i]);
      polyhedronPointsIds.push_back(i);
    }

    vtkIdType f1[4] = {0, 1, 2, 3};
    vtkIdType f2[4] = {7, 6, 5, 4};
    vtkIdType f3[4] = {0, 3, 7, 4};
    vtkIdType f4[4] = {5, 1, 0, 4};
    vtkIdType f5[4] = {6, 2, 1, 5};
    vtkIdType f6[4] = {7, 3, 2, 6};

    vtkIdType* cubeShapeFace[nFaces] = {f1,f2,f3,f4,f5,f6};

    for (int i = 0; i < nFaces; i++)
    {
      polyhedronFaces->InsertNextCell(4,cubeShapeFace[i]);
    }
  }
  else if (shape == colinear_cube)
  {
    // create a cube with two rectangles comprising one of the faces.
    const int nPoints = 10;
    const int nFaces = 7;

    double cubeShapePoint[nPoints][3] = {{.5,   .5,  .5},
                                         {0.,   .5,  .5},
                                         {-.5,  .5,  .5},
                                         {-.5, -.5,  .5},
                                         {.5,  -.5,  .5},
                                         {.5,   .5, -.5},
                                         {0.,   .5, -.5},
                                         {-.5,  .5, -.5},
                                         {-.5, -.5, -.5},
                                         {.5,  -.5, -.5}};

    polyhedronPoints->SetNumberOfPoints(nPoints);
    for (int i = 0; i < nPoints; i++)
    {
      polyhedronPoints->SetPoint(i,cubeShapePoint[i]);
      polyhedronPointsIds.push_back(i);
    }

    vtkIdType f1[5] = {0, 1, 2, 3, 4};
    vtkIdType f2[5] = {9, 8, 7, 6, 5};
    vtkIdType f3[4] = {0, 4, 9, 5};
    vtkIdType f4[4] = {7, 2, 1, 6};
    vtkIdType f5[4] = {5, 6, 1, 0};
    vtkIdType f6[4] = {8, 3, 2, 7};
    vtkIdType f7[4] = {9, 4, 3, 8};

    vtkIdType* cubeShapeFace[nFaces] = {f1,f2,f3,f4,f5,f6, f7};

    for (int i = 0; i < nFaces; i++)
    {
      if (i < 2)
      {
        polyhedronFaces->InsertNextCell(5,cubeShapeFace[i]);
      }
      else
      {
        polyhedronFaces->InsertNextCell(4,cubeShapeFace[i]);
      }
    }
  }
  else if (shape == degenerate_cube)
  {
    // create a cube with two degenerate points.
    const int nPoints = 10;
    const int nFaces = 7;

    double cubeShapePoint[nPoints][3] = {{.5,   .5,  .5},
                                         {.5,   .5,  .5},
                                         {-.5,  .5,  .5},
                                         {-.5, -.5,  .5},
                                         {.5,  -.5,  .5},
                                         {.5,   .5, -.5},
                                         {.5,   .5, -.5},
                                         {-.5,  .5, -.5},
                                         {-.5, -.5, -.5},
                                         {.5,  -.5, -.5}};

    polyhedronPoints->SetNumberOfPoints(nPoints);
    for (int i = 0; i < nPoints; i++)
    {
      polyhedronPoints->SetPoint(i,cubeShapePoint[i]);
      polyhedronPointsIds.push_back(i);
    }

    vtkIdType f1[5] = {0, 1, 2, 3, 4};
    vtkIdType f2[5] = {9, 8, 7, 6, 5};
    vtkIdType f3[4] = {0, 4, 9, 5};
    vtkIdType f4[4] = {7, 2, 1, 6};
    vtkIdType f5[4] = {5, 6, 1, 0};
    vtkIdType f6[4] = {8, 3, 2, 7};
    vtkIdType f7[4] = {9, 4, 3, 8};

    vtkIdType* cubeShapeFace[nFaces] = {f1,f2,f3,f4,f5,f6, f7};

    for (int i = 0; i < nFaces; i++)
    {
      if (i < 2)
      {
        polyhedronFaces->InsertNextCell(5,cubeShapeFace[i]);
      }
      else
      {
        polyhedronFaces->InsertNextCell(4,cubeShapeFace[i]);
      }
    }
  }
  else if (shape == convex_pyramid)
  {
    // create a simple convex pyramid
    const int nPoints = 5;
    const int nFaces = 5;

    double pyramidShapePoint[nPoints][3] = {{0., 0., -.5},
                                         {0., 1., -.5},
                                         {1., 1., -.5},
                                         {1., 0., -.5},
                                         {.5, .5, .5}};

    polyhedronPoints->SetNumberOfPoints(nPoints);
    for (int i = 0; i < nPoints; i++)
    {
      polyhedronPoints->SetPoint(i,pyramidShapePoint[i]);
      polyhedronPointsIds.push_back(i);
    }

    vtkIdType f1[4] = {0, 1, 2, 3};
    vtkIdType f2[3] = {0, 4, 1};
    vtkIdType f3[3] = {1, 4, 2};
    vtkIdType f4[3] = {2, 4, 3};
    vtkIdType f5[3] = {3, 4, 0};

    vtkIdType* pyramidShapeFace[nFaces] = {f1,f2,f3,f4,f5};

    for (int i = 0; i < nFaces; i++)
    {
      polyhedronFaces->InsertNextCell((i == 0 ? 4 : 3),pyramidShapeFace[i]);
    }
  }
  else if (shape == nonconvex_pyramid)
  {
    // create a simple non-convex pyramid
    const int nPoints = 5;
    const int nFaces = 5;

    double pyramidShapePoint[nPoints][3] = {{0., 0., -.5},
                                         {0., 1., -.5},
                                         {.25, .25, -.5},
                                         {1., 0., -.5},
                                         {0., 0., .5}};

    polyhedronPoints->SetNumberOfPoints(nPoints);
    for (int i = 0; i < nPoints; i++)
    {
      polyhedronPoints->SetPoint(i,pyramidShapePoint[i]);
      polyhedronPointsIds.push_back(i);
    }

    vtkIdType f1[4] = {0, 1, 2, 3};
    vtkIdType f2[3] = {0, 4, 1};
    vtkIdType f3[3] = {1, 4, 2};
    vtkIdType f4[3] = {2, 4, 3};
    vtkIdType f5[3] = {3, 4, 0};

    vtkIdType* pyramidShapeFace[nFaces] = {f1,f2,f3,f4,f5};

    for (int i = 0; i < nFaces; i++)
    {
      polyhedronFaces->InsertNextCell((i == 0 ? 4 : 3),pyramidShapeFace[i]);
    }
  }

  vtkSmartPointer<vtkUnstructuredGrid> ugrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  ugrid->SetPoints(polyhedronPoints);
  ugrid->InsertNextCell(VTK_POLYHEDRON,
                        polyhedronPoints->GetNumberOfPoints(),
                        &polyhedronPointsIds[0],
                        polyhedronFaces->GetNumberOfCells(),
                        polyhedronFaces->GetPointer());

  vtkPolyhedron *polyhedron = static_cast<vtkPolyhedron*>(ugrid->GetCell(0));

  return polyhedron->IsConvex();
}

int TestPolyhedronConvexity(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  if (IsConvex(dodecahedron) != true)
  {
    cerr << "convex dodehecahedron incorrectly classified as non-convex" << std::endl;
    return EXIT_FAILURE;
  }

  if (IsConvex(u_shape) != false)
  {
    cerr << "non-convex u-shape incorrectly classified as convex" << std::endl;
    return EXIT_FAILURE;
  }

  if (IsConvex(cube) != true)
  {
    cerr << "convex cube incorrectly classified as non-convex" << std::endl;
    return EXIT_FAILURE;
  }

  if (IsConvex(colinear_cube) != true)
  {
    cerr << "convex colinear cube incorrectly classified as non-convex" << std::endl;
    return EXIT_FAILURE;
  }

  if (IsConvex(degenerate_cube) != true)
  {
    cerr << "convex degenerate cube incorrectly classified as non-convex" << std::endl;
    return EXIT_FAILURE;
  }

  if (IsConvex(convex_pyramid) != true)
  {
    cerr << "convex pyramid incorrectly classified as non-convex" << std::endl;
    return EXIT_FAILURE;
  }

  if (IsConvex(nonconvex_pyramid) != false)
  {
    cerr << "non-convex pyramid incorrectly classified as convex" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
