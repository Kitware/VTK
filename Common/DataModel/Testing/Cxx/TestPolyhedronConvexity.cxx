// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
#include "vtkStringToken.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

// Uncomment the next line to write data to disk.
#undef VTK_DBG_TEST

#ifdef VTK_DBG_TEST
#include "vtkGenericDataObjectWriter.h"
#include <sstream>

#include <iostream>

#endif

using namespace vtk::literals;

vtkCellStatus IsConvex(vtkStringToken shape)
{
  vtkSmartPointer<vtkPoints> polyhedronPoints = vtkSmartPointer<vtkPoints>::New();

  vtkSmartPointer<vtkCellArray> polyhedronFaces = vtkSmartPointer<vtkCellArray>::New();

  std::vector<vtkIdType> polyhedronPointsIds;

  switch (shape.GetId())
  {
    case "dodecahedron"_hash:
    {
      // create a dodecahedron
      constexpr int nPoints = 20;
      constexpr int nFaces = 12;

      double dodechedronPoint[nPoints][3] = {
        { 1.21412, 0, 1.58931 },
        { 0.375185, 1.1547, 1.58931 },
        { -0.982247, 0.713644, 1.58931 },
        { -0.982247, -0.713644, 1.58931 },
        { 0.375185, -1.1547, 1.58931 },
        { 1.96449, 0, 0.375185 },
        { 0.607062, 1.86835, 0.375185 },
        { -1.58931, 1.1547, 0.375185 },
        { -1.58931, -1.1547, 0.375185 },
        { 0.607062, -1.86835, 0.375185 },
        { 1.58931, 1.1547, -0.375185 },
        { -0.607062, 1.86835, -0.375185 },
        { -1.96449, 0, -0.375185 },
        { -0.607062, -1.86835, -0.375185 },
        { 1.58931, -1.1547, -0.375185 },
        { 0.982247, 0.713644, -1.58931 },
        { -0.375185, 1.1547, -1.58931 },
        { -1.21412, 0, -1.58931 },
        { -0.375185, -1.1547, -1.58931 },
        { 0.982247, -0.713644, -1.58931 },
      };
      polyhedronPoints->SetNumberOfPoints(nPoints);
      for (int i = 0; i < nPoints; i++)
      {
        polyhedronPoints->SetPoint(i, dodechedronPoint[i]);
        polyhedronPointsIds.push_back(i);
      }

      vtkIdType dodechedronFace[nFaces][5] = {
        { 0, 1, 2, 3, 4 },
        { 0, 5, 10, 6, 1 },
        { 1, 6, 11, 7, 2 },
        { 2, 7, 12, 8, 3 },
        { 3, 8, 13, 9, 4 },
        { 4, 9, 14, 5, 0 },
        { 15, 10, 5, 14, 19 },
        { 16, 11, 6, 10, 15 },
        { 17, 12, 7, 11, 16 },
        { 18, 13, 8, 12, 17 },
        { 19, 14, 9, 13, 18 },
        { 19, 18, 17, 16, 15 },
      };

      for (int i = 0; i < nFaces; i++)
      {
        polyhedronFaces->InsertNextCell(5, dodechedronFace[i]);
      }
    }
    break;
    case "u_shape"_hash:
    {
      // create a concave shape
      constexpr int nPoints = 16;
      constexpr int nFaces = 10;

      double concaveShapePoint[nPoints][3] = {
        { .5, -.5, .25 },
        { .5, .5, .25 },
        { .25, .5, .25 },
        { .25, -.25, .25 },
        { -.25, -.25, .25 },
        { -.25, .5, .25 },
        { -.5, .5, .25 },
        { -.5, -.5, .25 },
        { .5, -.5, -.25 },
        { .5, .5, -.25 },
        { .25, .5, -.25 },
        { .25, -.25, -.25 },
        { -.25, -.25, -.25 },
        { -.25, .5, -.25 },
        { -.5, .5, -.25 },
        { -.5, -.5, -.25 },
      };

      polyhedronPoints->SetNumberOfPoints(nPoints);
      for (int i = 0; i < nPoints; i++)
      {
        polyhedronPoints->SetPoint(i, concaveShapePoint[i]);
        polyhedronPointsIds.push_back(i);
      }

      vtkIdType f1[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
      vtkIdType f2[8] = { 15, 14, 13, 12, 11, 10, 9, 8 };
      vtkIdType f3[4] = { 0, 7, 15, 8 };
      vtkIdType f4[4] = { 1, 0, 8, 9 };
      vtkIdType f5[4] = { 2, 1, 9, 10 };
      vtkIdType f6[4] = { 3, 2, 10, 11 };
      vtkIdType f7[4] = { 4, 3, 11, 12 };
      vtkIdType f8[4] = { 5, 4, 12, 13 };
      vtkIdType f9[4] = { 6, 5, 13, 14 };
      vtkIdType f10[4] = { 7, 6, 14, 15 };

      vtkIdType* concaveShapeFace[nFaces] = { f1, f2, f3, f4, f5, f6, f7, f8, f9, f10 };

      for (int i = 0; i < nFaces; i++)
      {
        polyhedronFaces->InsertNextCell((i >= 2 ? 4 : 8), concaveShapeFace[i]);
      }
    }
    break;
    case "cube"_hash:
    {
      // create a cube
      constexpr int nPoints = 8;
      constexpr int nFaces = 6;

      double cubeShapePoint[nPoints][3] = {
        { .5, .5, .5 },
        { -.5, .5, .5 },
        { -.5, -.5, .5 },
        { .5, -.5, .5 },
        { .5, .5, -.5 },
        { -.5, .5, -.5 },
        { -.5, -.5, -.5 },
        { .5, -.5, -.5 },
      };

      polyhedronPoints->SetNumberOfPoints(nPoints);
      for (int i = 0; i < nPoints; i++)
      {
        polyhedronPoints->SetPoint(i, cubeShapePoint[i]);
        polyhedronPointsIds.push_back(i);
      }

      vtkIdType f1[4] = { 0, 1, 2, 3 };
      vtkIdType f2[4] = { 7, 6, 5, 4 };
      vtkIdType f3[4] = { 0, 3, 7, 4 };
      vtkIdType f4[4] = { 5, 1, 0, 4 };
      vtkIdType f5[4] = { 6, 2, 1, 5 };
      vtkIdType f6[4] = { 7, 3, 2, 6 };

      vtkIdType* cubeShapeFace[nFaces] = { f1, f2, f3, f4, f5, f6 };

      for (int i = 0; i < nFaces; i++)
      {
        polyhedronFaces->InsertNextCell(4, cubeShapeFace[i]);
      }
    }
    break;
    case "colinear_cube"_hash:
    {
      // create a cube with two rectangles comprising one of the faces.
      constexpr int nPoints = 10;
      constexpr int nFaces = 7;

      double cubeShapePoint[nPoints][3] = {
        { .5, .5, .5 },
        { 0., .5, .5 },
        { -.5, .5, .5 },
        { -.5, -.5, .5 },
        { .5, -.5, .5 },
        { .5, .5, -.5 },
        { 0., .5, -.5 },
        { -.5, .5, -.5 },
        { -.5, -.5, -.5 },
        { .5, -.5, -.5 },
      };

      polyhedronPoints->SetNumberOfPoints(nPoints);
      for (int i = 0; i < nPoints; i++)
      {
        polyhedronPoints->SetPoint(i, cubeShapePoint[i]);
        polyhedronPointsIds.push_back(i);
      }

      vtkIdType f1[5] = { 0, 1, 2, 3, 4 };
      vtkIdType f2[5] = { 9, 8, 7, 6, 5 };
      vtkIdType f3[4] = { 0, 4, 9, 5 };
      vtkIdType f4[4] = { 7, 2, 1, 6 };
      vtkIdType f5[4] = { 5, 6, 1, 0 };
      vtkIdType f6[4] = { 8, 3, 2, 7 };
      vtkIdType f7[4] = { 9, 4, 3, 8 };

      vtkIdType* cubeShapeFace[nFaces] = { f1, f2, f3, f4, f5, f6, f7 };

      for (int i = 0; i < nFaces; i++)
      {
        if (i < 2)
        {
          polyhedronFaces->InsertNextCell(5, cubeShapeFace[i]);
        }
        else
        {
          polyhedronFaces->InsertNextCell(4, cubeShapeFace[i]);
        }
      }
    }
    break;
    case "degenerate_cube"_hash:
    {
      // create a cube with two degenerate points.
      constexpr int nPoints = 10;
      constexpr int nFaces = 7;

      double cubeShapePoint[nPoints][3] = {
        { .5, .5, .5 },
        { .5, .5, .5 },
        { -.5, .5, .5 },
        { -.5, -.5, .5 },
        { .5, -.5, .5 },
        { .5, .5, -.5 },
        { .5, .5, -.5 },
        { -.5, .5, -.5 },
        { -.5, -.5, -.5 },
        { .5, -.5, -.5 },
      };

      polyhedronPoints->SetNumberOfPoints(nPoints);
      for (int i = 0; i < nPoints; i++)
      {
        polyhedronPoints->SetPoint(i, cubeShapePoint[i]);
        polyhedronPointsIds.push_back(i);
      }

      vtkIdType f1[5] = { 0, 1, 2, 3, 4 };
      vtkIdType f2[5] = { 9, 8, 7, 6, 5 };
      vtkIdType f3[4] = { 0, 4, 9, 5 };
      vtkIdType f4[4] = { 7, 2, 1, 6 };
      vtkIdType f5[4] = { 5, 6, 1, 0 };
      vtkIdType f6[4] = { 8, 3, 2, 7 };
      vtkIdType f7[4] = { 9, 4, 3, 8 };

      vtkIdType* cubeShapeFace[nFaces] = { f1, f2, f3, f4, f5, f6, f7 };

      for (int i = 0; i < nFaces; i++)
      {
        if (i < 2)
        {
          polyhedronFaces->InsertNextCell(5, cubeShapeFace[i]);
        }
        else
        {
          polyhedronFaces->InsertNextCell(4, cubeShapeFace[i]);
        }
      }
    }
    break;
    case "convex_pyramid"_hash:
    {
      // create a simple convex pyramid
      constexpr int nPoints = 5;
      constexpr int nFaces = 5;

      double pyramidShapePoint[nPoints][3] = {
        { 0., 0., -.5 },
        { 0., 1., -.5 },
        { 1., 1., -.5 },
        { 1., 0., -.5 },
        { .5, .5, .5 },
      };

      polyhedronPoints->SetNumberOfPoints(nPoints);
      for (int i = 0; i < nPoints; i++)
      {
        polyhedronPoints->SetPoint(i, pyramidShapePoint[i]);
        polyhedronPointsIds.push_back(i);
      }

      vtkIdType f1[4] = { 0, 1, 2, 3 };
      vtkIdType f2[3] = { 0, 4, 1 };
      vtkIdType f3[3] = { 1, 4, 2 };
      vtkIdType f4[3] = { 2, 4, 3 };
      vtkIdType f5[3] = { 3, 4, 0 };

      vtkIdType* pyramidShapeFace[nFaces] = { f1, f2, f3, f4, f5 };

      for (int i = 0; i < nFaces; i++)
      {
        polyhedronFaces->InsertNextCell((i == 0 ? 4 : 3), pyramidShapeFace[i]);
      }
    }
    break;
    case "nonconvex_pyramid"_hash:
    {
      // create a simple non-convex pyramid
      constexpr int nPoints = 5;
      constexpr int nFaces = 5;

      double pyramidShapePoint[nPoints][3] = {
        { 0., 0., -.5 },
        { 0., 1., -.5 },
        { .25, .25, -.5 },
        { 1., 0., -.5 },
        { 0., 0., .5 },
      };

      polyhedronPoints->SetNumberOfPoints(nPoints);
      for (int i = 0; i < nPoints; i++)
      {
        polyhedronPoints->SetPoint(i, pyramidShapePoint[i]);
        polyhedronPointsIds.push_back(i);
      }

      vtkIdType f1[4] = { 0, 1, 2, 3 };
      vtkIdType f2[3] = { 0, 4, 1 };
      vtkIdType f3[3] = { 1, 4, 2 };
      vtkIdType f4[3] = { 2, 4, 3 };
      vtkIdType f5[3] = { 3, 4, 0 };

      vtkIdType* pyramidShapeFace[nFaces] = { f1, f2, f3, f4, f5 };

      for (int i = 0; i < nFaces; i++)
      {
        polyhedronFaces->InsertNextCell((i == 0 ? 4 : 3), pyramidShapeFace[i]);
      }
    }
    break;
    case "convex_prism"_hash:
    {
      // create a simple convex prism
      constexpr int nPoints = 6;
      constexpr int nFaces = 5;

      double prismShapePoint[nPoints][3] = {
        { -41.6027, 0., 10.2556 },
        { -37.5, 0., 10.6045 },
        { -41.8135, 0., 13.8533 },
        { -41.6027, 4., 10.2556 },
        { -37.5, 4., 10.6045 },
        { -41.8135, 4., 13.8533 },
      };

      polyhedronPoints->SetNumberOfPoints(nPoints);
      for (int i = 0; i < nPoints; i++)
      {
        polyhedronPoints->SetPoint(i, prismShapePoint[i]);
        polyhedronPointsIds.push_back(i);
      }

      vtkIdType f1[3] = { 0, 1, 2 };
      vtkIdType f2[3] = { 3, 5, 4 };
      vtkIdType f3[4] = { 0, 3, 4, 1 };
      vtkIdType f4[4] = { 1, 4, 5, 2 };
      vtkIdType f5[4] = { 0, 2, 5, 3 };

      vtkIdType* prismShapeFace[nFaces] = { f1, f2, f3, f4, f5 };

      for (int i = 0; i < nFaces; i++)
      {
        polyhedronFaces->InsertNextCell((i < 2 ? 3 : 4), prismShapeFace[i]);
      }
    }
    break;
    default:
    {
      std::cerr << "ERROR: Unhandled shape \"" << shape.Data() << "\".\n";
      return vtkCellStatus::Nonconvex;
    }
  }

  vtkSmartPointer<vtkUnstructuredGrid> ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  ugrid->SetPoints(polyhedronPoints);
  ugrid->InsertNextCell(VTK_POLYHEDRON, polyhedronPoints->GetNumberOfPoints(),
    polyhedronPointsIds.data(), polyhedronFaces);

  vtkPolyhedron* polyhedron = static_cast<vtkPolyhedron*>(ugrid->GetCell(0));

#ifdef VTK_DBG_TEST
  vtkNew<vtkGenericDataObjectWriter> wri;
  wri->SetInputDataObject(ugrid);
  std::ostringstream fname;
  fname << "polyhedron-" << shape.Data() << ".vtk";
  wri->SetFileName(fname.str().c_str());
  wri->Write();
#endif
  return polyhedron->IsConvex(0.1);
}

int TestPolyhedronConvexity(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  // clang-format off
  std::vector<std::pair<vtkStringToken, vtkCellStatus>> tests = {
    { "dodecahedron",      vtkCellStatus::Valid },
    { "u_shape",           vtkCellStatus::Nonconvex },
    { "cube",              vtkCellStatus::Valid },
    { "colinear_cube",     vtkCellStatus::Valid },
    { "degenerate_cube",   vtkCellStatus::DegenerateFaces },
    { "convex_pyramid",    vtkCellStatus::Valid },
    { "nonconvex_pyramid", vtkCellStatus::Nonconvex },
    { "convex_prism",      vtkCellStatus::Valid }
  };
  // clang-format on

  vtkCellStatus status;
  for (const auto& entry : tests)
  {
    status = IsConvex(entry.first);
    if (status != entry.second)
    {
      std::cerr << "ERROR: Shape " << entry.first.Data() << " classified " << status
                << ", expected " << entry.second << ".\n";
      return EXIT_FAILURE;
    }
    std::cout << "Shape " << entry.first.Data() << " classified " << status << ", expected "
              << entry.second << ".\n";
  }

  return EXIT_SUCCESS;
}
