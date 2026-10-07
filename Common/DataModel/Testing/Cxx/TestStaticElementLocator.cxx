// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests vtkStaticElementLocator with EdgeTuple, TriangleTuple and QuadTuple: BuildLocator() and
// IsInsertedElement(), including queries for absent elements, GroupElements(), and the data of
// the tuples.

#include "vtkStaticElementLocator.h"
#include "vtkType.h" // For vtkIdType

#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace
{
#define CHECK(condition)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(condition))                                                                              \
    {                                                                                              \
      std::cerr << "Check failed at line " << __LINE__ << ": " #condition << std::endl;            \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

bool TestEdges()
{
  using Edge = EdgeTuple<vtkIdType>;
  using LocatorType = vtkStaticElementLocator<vtkIdType, Edge>;

  // A small edge set with a gap: vertex 0 connects to 1 and 2 only, and a disjoint edge (7,9)
  // follows in sorted order.
  std::vector<Edge> edges = { { 0, 1 }, { 0, 2 }, { 7, 9 }, { 2, 1 }, { 9, 7 } };
  LocatorType locator;
  CHECK(locator.BuildLocator(static_cast<vtkIdType>(edges.size()), edges.data()) == 4);
  CHECK(locator.GetNumberOfElements() == 4);

  // Present edges are found with their points in any order, and their id refers to the edge.
  const std::vector<std::pair<vtkIdType, vtkIdType>> present = { { 0, 1 }, { 1, 0 }, { 0, 2 },
    { 1, 2 }, { 7, 9 }, { 9, 7 } };
  for (const auto& p : present)
  {
    const vtkIdType id = locator.IsInsertedElement(p.first, p.second);
    CHECK(id >= 0 && locator.GetElement(id).IsEdge(p.first, p.second));
  }

  // Absent edges: past every V1 of their V0, in the last V0 group, between existing V1 values,
  // with an absent V0, and out of the key range.
  const std::vector<std::pair<vtkIdType, vtkIdType>> absent = { { 0, 9 }, { 0, 5 }, { 7, 10 },
    { 1, 3 }, { 3, 4 }, { 10, 11 } };
  for (const auto& p : absent)
  {
    CHECK(locator.IsInsertedElement(Edge(p.first, p.second)) < 0);
  }

  // A single edge: the queries past its V1 do not scan past the end of the array.
  std::vector<Edge> single = { { 4, 6 } };
  LocatorType singleLocator;
  singleLocator.BuildLocator(1, single.data());
  CHECK(singleLocator.IsInsertedElement(6, 4) == 0);
  CHECK(singleLocator.IsInsertedElement(4, 8) < 0 && singleLocator.IsInsertedElement(4, 5) < 0);

  // An empty locator finds nothing.
  LocatorType empty;
  CHECK(empty.BuildLocator(0, nullptr) == 0 && empty.IsInsertedElement(0, 1) < 0);
  return true;
}

// The faces of two hexahedra sharing the face (1, 2, 6, 5), given with different orderings, and
// the triangles of a tetrahedron with a duplicate.
bool TestTrianglesAndQuads()
{
  using Quad = QuadTuple<vtkIdType>;
  std::vector<Quad> quads = { { 0, 3, 2, 1 }, { 4, 5, 6, 7 }, { 0, 1, 5, 4 }, { 1, 2, 6, 5 },
    { 2, 3, 7, 6 }, { 3, 0, 4, 7 }, { 1, 2, 9, 8 }, { 5, 10, 11, 6 }, { 1, 8, 10, 5 },
    { 8, 9, 11, 10 }, { 9, 2, 6, 11 }, { 2, 1, 5, 6 } };
  vtkStaticElementLocator<vtkIdType, Quad> quadLocator;
  CHECK(quadLocator.BuildLocator(static_cast<vtkIdType>(quads.size()), quads.data()) == 11);
  for (const Quad& q : { Quad(6, 5, 2, 1), Quad(3, 2, 1, 0), Quad(11, 10, 9, 8) })
  {
    const vtkIdType id = quadLocator.IsInsertedElement(q);
    CHECK(id >= 0 && quadLocator.GetElement(id) == q);
    CHECK(quadLocator.IsInsertedElement(q.V3, q.V2, q.V1, q.V0) == id);
  }
  // Quads sharing the first point or three points with an inserted one, and out of range.
  for (const Quad& q :
    { Quad(1, 2, 6, 7), Quad(0, 1, 2, 4), Quad(1, 5, 9, 11), Quad(12, 13, 14, 15) })
  {
    CHECK(quadLocator.IsInsertedElement(q) < 0);
  }

  using Triangle = TriangleTuple<vtkIdType>;
  std::vector<Triangle> triangles = { { 0, 1, 2 }, { 0, 3, 1 }, { 1, 3, 2 }, { 2, 3, 0 },
    { 2, 1, 0 } };
  vtkStaticElementLocator<vtkIdType, Triangle> triangleLocator;
  CHECK(
    triangleLocator.BuildLocator(static_cast<vtkIdType>(triangles.size()), triangles.data()) == 4);
  CHECK(triangleLocator.IsInsertedElement(3, 2, 1) >= 0 &&
    triangleLocator.IsInsertedElement(1, 0, 2) >= 0);
  CHECK(triangleLocator.IsInsertedElement(0, 1, 4) < 0);
  return true;
}

// GroupElements(): the groups of equal edges, whose data, not compared, gives their original ids,
// and which may outnumber IDType.
bool TestGroupElements()
{
  // Without data, the empty base of the tuple takes no space.
  static_assert(sizeof(EdgeTuple<int>) == 2 * sizeof(int), "an edge without data has no data");

  using Edge = EdgeTuple<vtkIdType, vtkIdType>;
  std::vector<Edge> edges;
  for (vtkIdType i = 0; i < 300; ++i)
  {
    // Every edge twice, with the points in either order, and its index as data.
    edges.emplace_back(i % 100, i % 100 + 1 + i % 3, 2 * i);
    edges.emplace_back(i % 100 + 1 + i % 3, i % 100, 2 * i + 1);
  }
  vtkStaticElementLocator<vtkIdType, Edge> locator;
  const auto& groups = locator.GroupElements(static_cast<vtkIdType>(edges.size()), edges.data());
  CHECK(groups.GetNumberOfGroups() == 300);
  CHECK(groups.GetGroupOffset(0) == 0 && groups.GetGroupOffset(300) == 600);
  for (vtkIdType g = 0; g < groups.GetNumberOfGroups(); ++g)
  {
    const Edge& first = edges[groups.GetGroupOffset(g)];
    CHECK(groups.GetGroupSize(g) == 2);
    // The two copies of an edge are equal, whatever their data, 2 i and 2 i + 1.
    const Edge& second = edges[groups.GetGroupOffset(g) + 1];
    CHECK(second == first && second.Data / 2 == first.Data / 2);
  }

  // There can be more elements than IDType holds: the groups are counted with vtkIdType.
  using SmallEdge = EdgeTuple<signed char>;
  std::vector<SmallEdge> smallEdges(200, SmallEdge(1, 2));
  vtkStaticElementLocator<signed char, SmallEdge> smallLocator;
  const auto& smallGroups =
    smallLocator.GroupElements(static_cast<vtkIdType>(smallEdges.size()), smallEdges.data());
  CHECK(smallGroups.GetNumberOfGroups() == 1 && smallGroups.GetGroupSize(0) == 200);
  return true;
}
} // namespace

int TestStaticElementLocator(int, char*[])
{
  if (!TestEdges() || !TestTrianglesAndQuads() || !TestGroupElements())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
