// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*
Test for vtkQuadricDecimation with degenerate triangle in the input.
This test creates a vtkPolyData with four triangles, one of which
is degenerate, and checks that the vtkQuadricDecimation filter
processes it without crashing or producing invalid output.
*/

#include <vtkLogger.h>
#include <vtkPlaneSource.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkQuadricDecimation.h>
#include <vtkSmartPointer.h>
#include <vtkTriangleFilter.h>

#include <cassert>
#include <cstdlib>

int TestQuadricDecimationDegenerateTriangle(int, char*[])
{
  // Create a plane source
  vtkSmartPointer<vtkPlaneSource> planeSource = vtkSmartPointer<vtkPlaneSource>::New();
  planeSource->SetXResolution(100);
  planeSource->SetYResolution(100);
  planeSource->Update();

  // Use vtkTriangleFilter to ensure all faces are triangles
  vtkSmartPointer<vtkTriangleFilter> triangleFilter = vtkSmartPointer<vtkTriangleFilter>::New();
  triangleFilter->SetInputConnection(planeSource->GetOutputPort());
  triangleFilter->Update();

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->ShallowCopy(triangleFilter->GetOutput());

  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  // Add a degenerate triangle (two points are the same)
  vtkIdType degenerateIds[3] = { 0, 0, 1 }; // Points 0, 0, 1 (degenerate)
  polyData->InsertNextCell(VTK_TRIANGLE, 3, degenerateIds);

  // Apply vtkQuadricDecimation
  vtkSmartPointer<vtkQuadricDecimation> decimate = vtkSmartPointer<vtkQuadricDecimation>::New();
  decimate->SetInputData(polyData);
  decimate->SetTargetReduction(0.5); // Reduce by 50%
  decimate->Update();
  vtkPolyData* decimated = decimate->GetOutput();

  // Check that the output is valid and does not contain degenerate triangles
  bool success = true;
  vtkSmartPointer<vtkIdList> ids = vtkSmartPointer<vtkIdList>::New();
  decimated->GetPolys()->InitTraversal();
  while (decimated->GetPolys()->GetNextCell(ids))
  {
    if (ids->GetNumberOfIds() != 3)
    {
      success = false;
      vtkLog(ERROR, "Found a polygon with " << ids->GetNumberOfIds() << " points instead of 3.");
      break;
    }
  }
  return (success ? EXIT_SUCCESS : EXIT_FAILURE);
}
