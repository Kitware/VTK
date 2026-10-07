// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkStaticElementLocator with edges and quads, in particular IsInsertedElement()
// queries for elements that are absent from the locator.

#include "vtkStaticElementLocator.h"
#include "vtkType.h" // For vtkIdType

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace
{
// Edge (V0, V1) with V0 < V1.
struct EdgeTuple
{
  vtkIdType V0 = 0;
  vtkIdType V1 = 0;

  EdgeTuple() = default;
  EdgeTuple(vtkIdType v0, vtkIdType v1)
    : V0(std::min(v0, v1))
    , V1(std::max(v0, v1))
  {
  }

  vtkIdType GetKey() const { return this->V0; }
  bool operator==(const EdgeTuple& e) const { return this->V0 == e.V0 && this->V1 == e.V1; }
  bool operator!=(const EdgeTuple& e) const { return !(*this == e); }
  bool operator<(const EdgeTuple& e) const
  {
    return this->V0 < e.V0 || (this->V0 == e.V0 && this->V1 < e.V1);
  }
};

// Quad face with sorted point ids, as used by vtkRepairTJunctions to match faces.
struct QuadTuple
{
  vtkIdType V[4] = { 0, 0, 0, 0 };

  QuadTuple() = default;
  QuadTuple(vtkIdType v0, vtkIdType v1, vtkIdType v2, vtkIdType v3)
    : V{ v0, v1, v2, v3 }
  {
    std::sort(this->V, this->V + 4);
  }

  vtkIdType GetKey() const { return this->V[0]; }
  bool operator==(const QuadTuple& q) const { return std::equal(this->V, this->V + 4, q.V); }
  bool operator!=(const QuadTuple& q) const { return !(*this == q); }
  bool operator<(const QuadTuple& q) const
  {
    return std::lexicographical_compare(this->V, this->V + 4, q.V, q.V + 4);
  }
};

bool TestEdges()
{
  using LocatorType = vtkStaticElementLocator<vtkIdType, EdgeTuple>;

  // A small edge set with a gap: vertex 0 connects to 1 and 2 only, and a disjoint edge (7,9)
  // follows in sorted order.
  std::vector<EdgeTuple> edges = { { 0, 1 }, { 0, 2 }, { 7, 9 }, { 2, 1 }, { 9, 7 } };

  LocatorType locator;
  if (locator.BuildLocator(static_cast<vtkIdType>(edges.size()), edges.data()) != 4 ||
    locator.GetNumberOfElements() != 4)
  {
    std::cerr << "duplicate edges were not removed" << std::endl;
    return false;
  }

  // Present edges must be found, regardless of the vertex order used in the query, and the
  // returned id must refer to the edge.
  const std::vector<std::pair<vtkIdType, vtkIdType>> present = { { 0, 1 }, { 1, 0 }, { 0, 2 },
    { 1, 2 }, { 7, 9 }, { 9, 7 } };
  for (const auto& p : present)
  {
    const EdgeTuple query(p.first, p.second);
    const vtkIdType id = locator.IsInsertedElement(query);
    if (id < 0 || locator.GetElement(id) != query)
    {
      std::cerr << "inserted edge (" << p.first << "," << p.second << ") was not found"
                << std::endl;
      return false;
    }
  }

  // Absent edges: whose second vertex is larger than every V1 stored for its first vertex, in
  // the last V0 group, between existing V1 values, with an absent V0, and out of the key range.
  const std::vector<std::pair<vtkIdType, vtkIdType>> absent = { { 0, 9 }, { 0, 5 }, { 7, 10 },
    { 1, 3 }, { 3, 4 }, { 10, 11 } };
  for (const auto& p : absent)
  {
    if (locator.IsInsertedElement(EdgeTuple(p.first, p.second)) >= 0)
    {
      std::cerr << "absent edge (" << p.first << "," << p.second << ") was reported as inserted"
                << std::endl;
      return false;
    }
  }

  // A locator over a single edge: queries beyond its V1 range must not scan past the end of the
  // array. The queries are constructed in place from their point ids.
  std::vector<EdgeTuple> single = { { 4, 6 } };
  LocatorType locator2;
  locator2.BuildLocator(1, single.data());
  if (locator2.IsInsertedElement(6, 4) < 0 || locator2.IsInsertedElement(4, 8) >= 0 ||
    locator2.IsInsertedElement(4, 5) >= 0)
  {
    std::cerr << "wrong query result for a single edge" << std::endl;
    return false;
  }

  // An empty locator finds nothing.
  LocatorType empty;
  if (empty.BuildLocator(0, nullptr) != 0 || empty.IsInsertedElement(EdgeTuple(0, 1)) >= 0)
  {
    std::cerr << "an empty locator found an edge" << std::endl;
    return false;
  }
  return true;
}

bool TestQuads()
{
  using LocatorType = vtkStaticElementLocator<vtkIdType, QuadTuple>;

  // The faces of two hexahedra sharing the face (1, 2, 6, 5), given with different orderings.
  // Point ids: the hexahedra are (0, 1, 2, 3, 4, 5, 6, 7) and (1, 8, 9, 2, 5, 10, 11, 6).
  std::vector<QuadTuple> quads = { { 0, 3, 2, 1 }, { 4, 5, 6, 7 }, { 0, 1, 5, 4 }, { 1, 2, 6, 5 },
    { 2, 3, 7, 6 }, { 3, 0, 4, 7 }, { 1, 2, 9, 8 }, { 5, 10, 11, 6 }, { 1, 8, 10, 5 },
    { 8, 9, 11, 10 }, { 9, 2, 6, 11 }, { 2, 1, 5, 6 } };

  LocatorType locator;
  if (locator.BuildLocator(static_cast<vtkIdType>(quads.size()), quads.data()) != 11)
  {
    std::cerr << "the shared quad was not merged" << std::endl;
    return false;
  }

  // Every face is found, with any ordering of its points.
  const std::vector<QuadTuple> present = { { 6, 5, 2, 1 }, { 3, 2, 1, 0 }, { 11, 10, 9, 8 },
    { 7, 6, 5, 4 } };
  for (const QuadTuple& q : present)
  {
    const vtkIdType id = locator.IsInsertedElement(q);
    if (id < 0 || locator.GetElement(id) != q ||
      locator.IsInsertedElement(q.V[3], q.V[2], q.V[1], q.V[0]) != id)
    {
      std::cerr << "an inserted quad was not found" << std::endl;
      return false;
    }
  }

  // Quads sharing the first point or three points with an inserted one, and out of range.
  const std::vector<QuadTuple> absent = { { 1, 2, 6, 7 }, { 0, 1, 2, 4 }, { 1, 5, 9, 11 },
    { 12, 13, 14, 15 } };
  for (const QuadTuple& q : absent)
  {
    if (locator.IsInsertedElement(q) >= 0)
    {
      std::cerr << "an absent quad was reported as inserted" << std::endl;
      return false;
    }
  }
  return true;
}
} // namespace

int TestStaticElementLocator(int, char*[])
{
  if (!TestEdges() || !TestQuads())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
