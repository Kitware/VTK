// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkStaticEdgeLocatorTemplate, in particular IsInsertedEdge()
// queries for edges that are absent from the locator.

#include "vtkStaticEdgeLocatorTemplate.h"
#include "vtkType.h" // For vtkIdType

#include <cstdlib>
#include <iostream>
#include <vector>

int TestStaticEdgeLocatorTemplate(int, char*[])
{
  using LocatorType = vtkStaticEdgeLocatorTemplate<vtkIdType, vtkIdType>;
  using EdgeType = LocatorType::EdgeTupleType;

  // A small edge set with a gap: vertex 0 connects to 1 and 2 only,
  // and a disjoint edge (7,9) follows in sorted order.
  std::vector<EdgeType> edges;
  edges.emplace_back(0, 1, 0);
  edges.emplace_back(0, 2, 0);
  edges.emplace_back(7, 9, 0);
  edges.emplace_back(2, 1, 0); // canonicalized to (1,2)
  edges.emplace_back(9, 7, 0); // canonicalized to (7,9): duplicate

  LocatorType locator;
  locator.BuildLocator(static_cast<vtkIdType>(edges.size()), edges.data());

  // Present edges must be found, regardless of the vertex order used
  // in the query.
  if (locator.IsInsertedEdge(0, 1) < 0 || locator.IsInsertedEdge(1, 0) < 0 ||
    locator.IsInsertedEdge(0, 2) < 0 || locator.IsInsertedEdge(1, 2) < 0 ||
    locator.IsInsertedEdge(7, 9) < 0 || locator.IsInsertedEdge(9, 7) < 0)
  {
    std::cerr << "an inserted edge was not found" << std::endl;
    return EXIT_FAILURE;
  }

  // Absent edge whose second vertex is larger than every V1 stored for
  // its first vertex. The V1 scan previously ran into the following
  // V0 group (matching the unrelated edge (7,9)) and, when no such
  // group followed, past the end of the edge array.
  if (locator.IsInsertedEdge(0, 9) >= 0)
  {
    std::cerr << "absent edge (0,9) was reported as inserted" << std::endl;
    return EXIT_FAILURE;
  }
  if (locator.IsInsertedEdge(0, 5) >= 0)
  {
    std::cerr << "absent edge (0,5) was reported as inserted" << std::endl;
    return EXIT_FAILURE;
  }

  // Absent edge in the last V0 group: the scan has no following group
  // and must stop at the end of the array.
  if (locator.IsInsertedEdge(7, 10) >= 0)
  {
    std::cerr << "absent edge (7,10) was reported as inserted" << std::endl;
    return EXIT_FAILURE;
  }

  // Absent edges between existing V1 values and with absent V0.
  if (locator.IsInsertedEdge(1, 3) >= 0 || locator.IsInsertedEdge(3, 4) >= 0)
  {
    std::cerr << "an absent edge was reported as inserted" << std::endl;
    return EXIT_FAILURE;
  }

  // A locator over a single-vertex group: queries beyond its V1 range
  // must not scan past the end of the array.
  std::vector<EdgeType> single;
  single.emplace_back(4, 6, 0);
  LocatorType locator2;
  locator2.BuildLocator(static_cast<vtkIdType>(single.size()), single.data());
  if (locator2.IsInsertedEdge(4, 6) < 0)
  {
    std::cerr << "edge (4,6) was not found" << std::endl;
    return EXIT_FAILURE;
  }
  if (locator2.IsInsertedEdge(4, 8) >= 0 || locator2.IsInsertedEdge(4, 5) >= 0)
  {
    std::cerr << "an absent edge was reported as inserted (single)" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
