// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// vtkPolygon::EarCutTriangulation() on a folded polygon, an iso-polygon of a polyhedron of a
// hanging-node mesh, on which ear cutting stalls with two triangles left. It reports the
// failure, and the triangles still cover the polygon: every edge of the polygon is the edge of
// exactly one triangle, and every diagonal of two.

#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <utility>

int TestPolygonEarCutStall(int, char*[])
{
  const double x[8][3] = {
    { 4, 7, 8.2828989028930664 },
    { 4, 7.7171010971069336, 9 },
    { 4, 8, 9.0968866348266602 },
    { 4.3637428283691406, 8, 10 },
    { 4.5061774253845215, 7, 10 },
    { 4.9564151763916016, 6, 10 },
    { 4.383540153503418, 6, 8 },
    { 4, 6.9031133651733398, 8 },
  };
  const vtkIdType n = 8;
  vtkNew<vtkPolygon> polygon;
  polygon->GetPoints()->SetNumberOfPoints(n);
  polygon->GetPointIds()->SetNumberOfIds(n);
  for (vtkIdType i = 0; i < n; ++i)
  {
    polygon->GetPoints()->SetPoint(i, x[i]);
    polygon->GetPointIds()->SetId(i, i);
  }

  vtkNew<vtkIdList> triangles;
  const int success = polygon->EarCutTriangulation(triangles);
  int status = EXIT_SUCCESS;
  if (success)
  {
    std::cerr << "This polygon was expected to stall the ear cutting" << std::endl;
    status = EXIT_FAILURE;
  }
  if (triangles->GetNumberOfIds() != 3 * (n - 2))
  {
    std::cerr << triangles->GetNumberOfIds() / 3 << " triangles instead of " << n - 2 << std::endl;
    return EXIT_FAILURE;
  }

  std::map<std::pair<vtkIdType, vtkIdType>, int> edgeUses;
  for (vtkIdType t = 0; t < triangles->GetNumberOfIds(); t += 3)
  {
    for (int k = 0; k < 3; ++k)
    {
      vtkIdType a = triangles->GetId(t + k);
      vtkIdType b = triangles->GetId(t + (k + 1) % 3);
      ++edgeUses[std::minmax(a, b)];
    }
  }
  for (const auto& [edge, uses] : edgeUses)
  {
    const bool boundary =
      (edge.second - edge.first == 1) || (edge.first == 0 && edge.second == n - 1);
    if (uses != (boundary ? 1 : 2))
    {
      std::cerr << "edge (" << edge.first << ", " << edge.second << ") is used " << uses << " times"
                << std::endl;
      status = EXIT_FAILURE;
    }
  }
  for (vtkIdType i = 0; i < n; ++i)
  {
    if (edgeUses.count(std::minmax(i, (i + 1) % n)) == 0)
    {
      std::cerr << "edge (" << i << ", " << (i + 1) % n << ") is not covered" << std::endl;
      status = EXIT_FAILURE;
    }
  }
  return status;
}
