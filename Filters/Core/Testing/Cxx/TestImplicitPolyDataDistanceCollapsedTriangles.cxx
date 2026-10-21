// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Check vtkImplicitPolyDataDistance on a surface with triangles that have two coincident vertices.
// Merging the coincident points collapses such a triangle into a line. The distance evaluation only
// handles triangles, so the line must not reach it: reading a third point of a line reads past the
// end of its point ids.

#include "vtkCellArray.h"
#include "vtkDataArray.h"
#include "vtkDistancePolyDataFilter.h"
#include "vtkImplicitPolyDataDistance.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkSphereSource.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int TestImplicitPolyDataDistanceCollapsedTriangles(int, char*[])
{
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetRadius(10.0);
  sphereSource->SetThetaResolution(24);
  sphereSource->SetPhiResolution(24);
  sphereSource->Update();
  vtkPolyData* sphere = sphereSource->GetOutput();
  const vtkIdType numberOfSpherePoints = sphere->GetNumberOfPoints();

  // The same surface with a collapsed triangle at every vertex, which has a copy of the vertex as
  // its third point
  vtkNew<vtkPoints> points;
  points->DeepCopy(sphere->GetPoints());
  vtkNew<vtkCellArray> polys;
  polys->DeepCopy(sphere->GetPolys());
  for (vtkIdType pointId = 0; pointId < numberOfSpherePoints; ++pointId)
  {
    const vtkIdType copyId = points->InsertNextPoint(points->GetPoint(pointId));
    const vtkIdType neighborId = (pointId + 1) % numberOfSpherePoints;
    polys->InsertNextCell({ pointId, neighborId, copyId });
  }
  vtkNew<vtkPolyData> surface;
  surface->SetPoints(points);
  surface->SetPolys(polys);

  vtkNew<vtkImplicitPolyDataDistance> distance;
  distance->SetInput(surface);
  vtkNew<vtkImplicitPolyDataDistance> referenceDistance;
  referenceDistance->SetInput(sphere);

  // Query outside and inside each vertex, where the closest feature of the surface is the vertex
  // itself, and halfway between two vertices, where it is mostly an edge
  vtkNew<vtkPoints> queryPoints;
  for (vtkIdType pointId = 0; pointId < numberOfSpherePoints; ++pointId)
  {
    double vertex[3], neighbor[3], query[3];
    sphere->GetPoint(pointId, vertex);
    sphere->GetPoint((pointId + 1) % numberOfSpherePoints, neighbor);
    for (double scale : { 1.5, 0.5 })
    {
      for (int i = 0; i < 3; ++i)
      {
        query[i] = scale * vertex[i];
      }
      queryPoints->InsertNextPoint(query);
      for (int i = 0; i < 3; ++i)
      {
        query[i] = scale * 0.5 * (vertex[i] + neighbor[i]);
      }
      queryPoints->InsertNextPoint(query);
    }
  }

  // The expected distances are evaluated here, with the backend VTK is configured with. Evaluating
  // them after a change of backend would leave the thread-local cells of the reference distance in
  // the storage of that backend, which their destructor, run with the configured backend, misses.
  std::vector<double> expectedDistances(queryPoints->GetNumberOfPoints());
  int numberOfErrors = 0;
  for (vtkIdType queryId = 0; queryId < queryPoints->GetNumberOfPoints(); ++queryId)
  {
    double query[3];
    queryPoints->GetPoint(queryId, query);
    const double value = distance->EvaluateFunction(query);
    const double expected = referenceDistance->EvaluateFunction(query);
    expectedDistances[queryId] = expected;
    if (!std::isfinite(value) || std::abs(value - expected) > 1e-9)
    {
      if (numberOfErrors++ < 10)
      {
        std::cerr << "Distance at (" << query[0] << ", " << query[1] << ", " << query[2] << ") is "
                  << value << " instead of " << expected << std::endl;
      }
    }
  }

  // The same through vtkDistancePolyDataFilter, which evaluates the points in parallel
  vtkNew<vtkPolyData> queryPolyData;
  queryPolyData->SetPoints(queryPoints);
  vtkNew<vtkCellArray> queryVerts;
  for (vtkIdType queryId = 0; queryId < queryPoints->GetNumberOfPoints(); ++queryId)
  {
    queryVerts->InsertNextCell(1, &queryId);
  }
  queryPolyData->SetVerts(queryVerts);
  // The backend VTK is configured with, and the sequential one
  const std::string defaultBackend = vtkSMPTools::GetBackend();
  for (const std::string& backend : { defaultBackend, std::string("Sequential") })
  {
    vtkSMPTools::SetBackend(backend.c_str());
    vtkNew<vtkDistancePolyDataFilter> filter;
    filter->SetInputData(0, queryPolyData);
    filter->SetInputData(1, surface);
    filter->SetSignedDistance(true);
    filter->ComputeSecondDistanceOff();
    filter->Update();
    vtkDataArray* distances = filter->GetOutput()->GetPointData()->GetArray("Distance");
    for (vtkIdType queryId = 0; queryId < queryPoints->GetNumberOfPoints(); ++queryId)
    {
      const double expected = expectedDistances[queryId];
      const double value = distances ? distances->GetTuple1(queryId) : vtkMath::Nan();
      if (!std::isfinite(value) || std::abs(value - expected) > 1e-9)
      {
        if (numberOfErrors++ < 10)
        {
          std::cerr << "vtkDistancePolyDataFilter with the " << backend << " backend gives "
                    << value << " instead of " << expected << std::endl;
        }
      }
    }
  }
  vtkSMPTools::SetBackend(defaultBackend.c_str());

  if (numberOfErrors > 0)
  {
    std::cerr << numberOfErrors << " wrong distances" << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
