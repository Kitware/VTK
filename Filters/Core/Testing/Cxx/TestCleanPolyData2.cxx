/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCleanPolyData2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCellArray.h>
#include <vtkCleanPolyData.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>

namespace
{
vtkSmartPointer<vtkPolyData> ConstructLines()
{
  vtkIdType ptIds[4];
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 0.0, 0.0); // Repeated point 0

  vtkSmartPointer<vtkCellArray> degeneratedLines = vtkSmartPointer<vtkCellArray>::New();

  // Construct a non-degenerated line (0, 1)
  ptIds[0] = 0;
  ptIds[1] = 1;
  degeneratedLines->InsertNextCell(2, ptIds);

  // Construct a line (0, 0) degenerated to a vertex
  ptIds[0] = 0;
  ptIds[1] = 0;
  degeneratedLines->InsertNextCell(2, ptIds);

  // Construct a line (0, 3) degenerated to a vertex ONLY if merging is ON
  ptIds[0] = 0;
  ptIds[1] = 3;
  degeneratedLines->InsertNextCell(2, ptIds);

  // Construct a non-degenerate poly line
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[2] = 2;
  degeneratedLines->InsertNextCell(3, ptIds);

  // Construct a degenerate polyline
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[3] = 1;
  degeneratedLines->InsertNextCell(3, ptIds);

  // Construct a polyline that is degenerate to a vertex ONLY if point merging is ON
  ptIds[0] = 0;
  ptIds[1] = 3;
  ptIds[2] = 0;
  degeneratedLines->InsertNextCell(3, ptIds);

  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints(points);
  polydata->SetLines(degeneratedLines);

  return polydata;
}

vtkSmartPointer<vtkPolyData> ConstructPolys()
{
  vtkIdType ptIds[4];
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 1.0); // Unused
  points->InsertNextPoint(0.0, 0.0, 0.0); // Repeated point 0
  points->InsertNextPoint(1.0, 0.0, 0.0); // Repeated point 1

  vtkSmartPointer<vtkCellArray> degeneratedPolys = vtkSmartPointer<vtkCellArray>::New();

  // Construct a non-degenerated triangle (0, 1, 2)
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[2] = 2;
  degeneratedPolys->InsertNextCell(3, ptIds);

  // Construct a triangle (0, 0, 0) degenerated to a vertex
  ptIds[0] = 0;
  ptIds[1] = 0;
  ptIds[2] = 0;
  degeneratedPolys->InsertNextCell(3, ptIds);

  // Construct a triangle (0, 1, 1) degenerated to a line
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[2] = 1;
  degeneratedPolys->InsertNextCell(3, ptIds);

  // Construct a triangle (0, 1, 5) degenerated to a line ONLY if merging is ON
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[2] = 5;
  degeneratedPolys->InsertNextCell(3, ptIds);

  // Construct a triangle (0, 4, 0) degenerated to a vertex ONLY if merging is ON or degenerated to
  // a line if merging is OFF
  ptIds[0] = 0;
  ptIds[1] = 4;
  ptIds[2] = 0;
  degeneratedPolys->InsertNextCell(3, ptIds);

  // Construct a quad (1, 1, 1, 1) degenerated to a vertex
  ptIds[0] = 1;
  ptIds[1] = 1;
  ptIds[2] = 1;
  ptIds[3] = 1;
  degeneratedPolys->InsertNextCell(4, ptIds);

  // Construct a quad (0, 1, 1, 0) degenerated to a line
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[2] = 1;
  ptIds[3] = 0;
  degeneratedPolys->InsertNextCell(4, ptIds);

  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints(points);
  polydata->SetPolys(degeneratedPolys);

  return polydata;
}

vtkSmartPointer<vtkPolyData> ConstructStrips()
{
  vtkIdType ptIds[4];
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 1.0); // Unused
  points->InsertNextPoint(0.0, 0.0, 0.0); // Repeated point 0
  points->InsertNextPoint(1.0, 0.0, 0.0); // Repeated point 1
  points->InsertNextPoint(1.0, 1.0, 0.0); // Repeated point 2

  vtkSmartPointer<vtkCellArray> degeneratedStrips = vtkSmartPointer<vtkCellArray>::New();

  // Construct a non-degenerated strip (0, 1, 2, 3)
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[2] = 2;
  ptIds[3] = 3;
  degeneratedStrips->InsertNextCell(4, ptIds);

  // Construct a strip (0, 1, 2, 2) degenerated to a triangle
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[2] = 2;
  ptIds[3] = 2;
  degeneratedStrips->InsertNextCell(4, ptIds);

  // Construct a strip (0, 1, 2, 7) degenerated to a triangle ONLY if merging is ON
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[2] = 2;
  ptIds[3] = 7;
  degeneratedStrips->InsertNextCell(4, ptIds);

  // Construct a strip (0, 1, 1, 1) degenerated to a line
  ptIds[0] = 0;
  ptIds[1] = 1;
  ptIds[2] = 1;
  ptIds[3] = 1;
  degeneratedStrips->InsertNextCell(4, ptIds);

  // Construct a strip (0, 0, 6, 5) degenerated to a line ONLY if merging is ON or degenerated to a
  // triangle if merging is OFF
  ptIds[0] = 0;
  ptIds[1] = 0;
  ptIds[2] = 6;
  ptIds[3] = 5;
  degeneratedStrips->InsertNextCell(4, ptIds);

  // Construct a strip (2, 2, 2, 2) degenerated to a vertex
  ptIds[0] = 2;
  ptIds[1] = 2;
  ptIds[2] = 2;
  ptIds[3] = 2;
  degeneratedStrips->InsertNextCell(4, ptIds);

  // Construct a strip (0, 0, 0, 5) degenerated to a vertex ONLY if merging is ON or degenerated to
  // a line if merging is OFF
  ptIds[0] = 0;
  ptIds[1] = 0;
  ptIds[2] = 0;
  ptIds[3] = 5;
  degeneratedStrips->InsertNextCell(4, ptIds);

  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->SetPoints(points);
  polydata->SetStrips(degeneratedStrips);

  return polydata;
}

bool UpdateAndTestCleanPolyData(vtkSmartPointer<vtkCleanPolyData> clean, int numExpectedPoints,
  int numExpectedVertices, int numExpectedLines, int numExpectedPolys, int numExpectedStrips)
{
  clean->Update();
  auto ds = clean->GetOutput();

  if (ds->GetNumberOfPoints() != numExpectedPoints)
  {
    std::cerr << "Expected " << numExpectedPoints << " but got " << ds->GetNumberOfPoints()
              << " points." << std::endl;
    return false;
  }

  if (ds->GetNumberOfVerts() != numExpectedVertices)
  {
    std::cerr << "Expected " << numExpectedVertices << " but got " << ds->GetNumberOfVerts()
              << " verts." << std::endl;
    return false;
  }

  if (ds->GetNumberOfLines() != numExpectedLines)
  {
    std::cerr << "Expected " << numExpectedLines << " but got " << ds->GetNumberOfLines()
              << " lines." << std::endl;
    return false;
  }

  if (ds->GetNumberOfPolys() != numExpectedPolys)
  {
    std::cerr << "Expected " << numExpectedPolys << " but got " << ds->GetNumberOfPolys()
              << " polys." << std::endl;
    return false;
  }

  if (ds->GetNumberOfStrips() != numExpectedStrips)
  {
    std::cerr << "Expected " << numExpectedStrips << " but got " << ds->GetNumberOfStrips()
              << " strips." << std::endl;
    return false;
  }

  return true;
}
}

int TestCleanPolyData2(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  auto lines = ConstructLines();
  auto polys = ConstructPolys();
  auto strips = ConstructStrips();

  // First test degenerate conversions without merging
  vtkSmartPointer<vtkCleanPolyData> clean = vtkSmartPointer<vtkCleanPolyData>::New();
  clean->PointMergingOff();
  clean->ConvertLinesToPointsOn();
  clean->ConvertPolysToLinesOn();
  clean->ConvertStripsToPolysOn();

  clean->SetInputData(lines);
  if (!UpdateAndTestCleanPolyData(clean, 4, 1, 5, 0, 0))
  {
    return EXIT_FAILURE;
  }

  clean->SetInputData(polys);
  if (!UpdateAndTestCleanPolyData(clean, 5, 2, 3, 2, 0))
  {
    return EXIT_FAILURE;
  }

  clean->SetInputData(strips);
  if (!UpdateAndTestCleanPolyData(clean, 7, 1, 2, 2, 2))
  {
    return EXIT_FAILURE;
  }

  // Now test degenerate elimination without merging
  clean->ConvertLinesToPointsOff();
  clean->ConvertPolysToLinesOff();
  clean->ConvertStripsToPolysOff();

  clean->SetInputData(lines);
  if (!UpdateAndTestCleanPolyData(clean, 4, 0, 5, 0, 0))
  {
    return EXIT_FAILURE;
  }

  clean->SetInputData(polys);
  if (!UpdateAndTestCleanPolyData(clean, 5, 0, 0, 2, 0))
  {
    return EXIT_FAILURE;
  }

  clean->SetInputData(strips);
  if (!UpdateAndTestCleanPolyData(clean, 7, 0, 0, 0, 2))
  {
    return EXIT_FAILURE;
  }

  // Now test degenerate conversion with merging
  clean->PointMergingOn();
  clean->ConvertLinesToPointsOn();
  clean->ConvertPolysToLinesOn();
  clean->ConvertStripsToPolysOn();

  clean->SetInputData(lines);
  if (!UpdateAndTestCleanPolyData(clean, 3, 3, 3, 0, 0))
  {
    return EXIT_FAILURE;
  }

  clean->SetInputData(polys);
  if (!UpdateAndTestCleanPolyData(clean, 3, 3, 3, 1, 0))
  {
    return EXIT_FAILURE;
  }

  clean->SetInputData(strips);
  if (!UpdateAndTestCleanPolyData(clean, 4, 2, 2, 2, 1))
  {
    return EXIT_FAILURE;
  }

  // Now test degenerate elimination with merging
  clean->ConvertLinesToPointsOff();
  clean->ConvertPolysToLinesOff();
  clean->ConvertStripsToPolysOff();

  clean->SetInputData(lines);
  if (!UpdateAndTestCleanPolyData(clean, 3, 0, 3, 0, 0))
  {
    return EXIT_FAILURE;
  }

  clean->SetInputData(polys);
  if (!UpdateAndTestCleanPolyData(clean, 3, 0, 0, 1, 0))
  {
    return EXIT_FAILURE;
  }

  clean->SetInputData(strips);
  if (!UpdateAndTestCleanPolyData(clean, 4, 0, 0, 0, 1))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
