//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_rendering_raytracing_Cell_Intersector_h
#define viskores_rendering_raytracing_Cell_Intersector_h

#include <viskores/CellShape.h>
#include <viskores/rendering/raytracing/CellTables.h>
#include <viskores/rendering/raytracing/TriangleIntersections.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{
//
// Any supported element. If the cell shape is not
// supported it does nothing, e.g. a 2D cellkk.
//
template <typename T>
VISKORES_EXEC_CONT inline void IntersectZoo(T xpoints[8],
                                            T ypoints[8],
                                            T zpoints[8],
                                            const viskores::Vec<T, 3>& dir,
                                            const viskores::Vec<T, 3>& origin,
                                            T distances[6],
                                            const viskores::Int32& shapeType)
{
  // Some precalc for water tight intersections
  viskores::Vec<T, 3> s;
  viskores::Vec3i_32 k;
  WaterTight intersector;
  intersector.FindDir(dir, s, k);
  CellTables tables;
  const viskores::Int32 tableOffset = tables.ZooLookUp(tables.CellTypeLookUp(shapeType), 0);
  const viskores::Int32 numTriangles = tables.ZooLookUp(tables.CellTypeLookUp(shapeType), 1);
  // Decompose each face into two triangles
  for (int i = 0; i < 6; ++i)
    distances[i] = -1.;
  for (int i = 0; i < numTriangles; ++i)
  {
    const viskores::Int32 offset = tableOffset + i;
    viskores::Vec<T, 3> a, c, b;
    a[0] = xpoints[tables.ZooTable(offset, 1)];
    a[1] = ypoints[tables.ZooTable(offset, 1)];
    a[2] = zpoints[tables.ZooTable(offset, 1)];
    b[0] = xpoints[tables.ZooTable(offset, 2)];
    b[1] = ypoints[tables.ZooTable(offset, 2)];
    b[2] = zpoints[tables.ZooTable(offset, 2)];
    c[0] = xpoints[tables.ZooTable(offset, 3)];
    c[1] = ypoints[tables.ZooTable(offset, 3)];
    c[2] = zpoints[tables.ZooTable(offset, 3)];
    const viskores::Int32 faceId = tables.ZooTable(offset, 0);
    T distance = -1.f;

    T uNotUsed, vNotUsed;
    intersector.IntersectTriSn(a, b, c, s, k, distance, uNotUsed, vNotUsed, origin);

    if (distance != -1.f)
    {
      if (distances[faceId] != -1.f)
        distances[faceId] = viskores::Min(distance, distances[faceId]);
      else
        distances[faceId] = distance;
    }
  }
}
template <typename T>
VISKORES_EXEC_CONT inline void IntersectHex(T xpoints[8],
                                            T ypoints[8],
                                            T zpoints[8],
                                            const viskores::Vec<T, 3>& dir,
                                            const viskores::Vec<T, 3>& origin,
                                            T distances[6])
{
  // Some precalc for water tight intersections
  viskores::Vec<T, 3> s;
  viskores::Vec3i_32 k;
  WaterTight intersector;
  intersector.FindDir(dir, s, k);

  CellTables tables;
  // Decompose each face into two triangles
  for (int i = 0; i < 6; ++i)
  {
    viskores::Vec<T, 3> a, c, b, d;
    a[0] = xpoints[tables.ShapesFaceList(i, 1)];
    a[1] = ypoints[tables.ShapesFaceList(i, 1)];
    a[2] = zpoints[tables.ShapesFaceList(i, 1)];
    b[0] = xpoints[tables.ShapesFaceList(i, 2)];
    b[1] = ypoints[tables.ShapesFaceList(i, 2)];
    b[2] = zpoints[tables.ShapesFaceList(i, 2)];
    c[0] = xpoints[tables.ShapesFaceList(i, 3)];
    c[1] = ypoints[tables.ShapesFaceList(i, 3)];
    c[2] = zpoints[tables.ShapesFaceList(i, 3)];
    d[0] = xpoints[tables.ShapesFaceList(i, 4)];
    d[1] = ypoints[tables.ShapesFaceList(i, 4)];
    d[2] = zpoints[tables.ShapesFaceList(i, 4)];
    T distance = -1.f;
    distances[i] = distance; //init to -1

    T uNotUsed, vNotUsed;
    intersector.IntersectTriSn(a, b, c, s, k, distance, uNotUsed, vNotUsed, origin);

    if (distance != -1.f)
      distances[i] = distance;

    distance = -1.f;

    intersector.IntersectTriSn(a, c, d, s, k, distance, uNotUsed, vNotUsed, origin);



    if (distance != -1.f)
    {
      if (distances[i] != -1.f)
        distances[i] = viskores::Min(distance, distances[i]);
      else
        distances[i] = distance;
    }
  }
}
template <typename T>
VISKORES_EXEC_CONT inline void IntersectTet(T xpoints[8],
                                            T ypoints[8],
                                            T zpoints[8],
                                            const viskores::Vec<T, 3>& dir,
                                            const viskores::Vec<T, 3>& origin,
                                            T distances[6])
{
  // Some precalc for water tight intersections
  viskores::Vec<T, 3> s;
  viskores::Vec3i_32 k;
  WaterTight intersector;
  intersector.FindDir(dir, s, k);

  CellTables tables;
  const viskores::Int32 tableOffset = tables.FaceLookUp(tables.CellTypeLookUp(CELL_SHAPE_TETRA), 0);
  for (viskores::Int32 i = 0; i < 4; ++i)
  {
    viskores::Vec<T, 3> a, c, b;
    a[0] = xpoints[tables.ShapesFaceList(i + tableOffset, 1)];
    a[1] = ypoints[tables.ShapesFaceList(i + tableOffset, 1)];
    a[2] = zpoints[tables.ShapesFaceList(i + tableOffset, 1)];
    b[0] = xpoints[tables.ShapesFaceList(i + tableOffset, 2)];
    b[1] = ypoints[tables.ShapesFaceList(i + tableOffset, 2)];
    b[2] = zpoints[tables.ShapesFaceList(i + tableOffset, 2)];
    c[0] = xpoints[tables.ShapesFaceList(i + tableOffset, 3)];
    c[1] = ypoints[tables.ShapesFaceList(i + tableOffset, 3)];
    c[2] = zpoints[tables.ShapesFaceList(i + tableOffset, 3)];
    T distance = -1.f;
    distances[i] = distance; //init to -1

    T uNotUsed, vNotUsed;

    intersector.IntersectTriSn(a, b, c, s, k, distance, uNotUsed, vNotUsed, origin);

    if (distance != -1.f)
      distances[i] = distance;
  }
}

//
// Wedge
//
template <typename T>
VISKORES_EXEC_CONT inline void IntersectWedge(T xpoints[8],
                                              T ypoints[8],
                                              T zpoints[8],
                                              const viskores::Vec<T, 3>& dir,
                                              const viskores::Vec<T, 3>& origin,
                                              T distances[6])
{
  // Some precalc for water tight intersections
  viskores::Vec<T, 3> s;
  viskores::Vec3i_32 k;
  WaterTight intersector;
  intersector.FindDir(dir, s, k);
  // TODO: try two sepate loops to see performance impact
  CellTables tables;
  const viskores::Int32 tableOffset = tables.FaceLookUp(tables.CellTypeLookUp(CELL_SHAPE_WEDGE), 0);
  // Decompose each face into two triangles
  for (int i = 0; i < 5; ++i)
  {
    viskores::Vec<T, 3> a, c, b, d;
    a[0] = xpoints[tables.ShapesFaceList(i + tableOffset, 1)];
    a[1] = ypoints[tables.ShapesFaceList(i + tableOffset, 1)];
    a[2] = zpoints[tables.ShapesFaceList(i + tableOffset, 1)];
    b[0] = xpoints[tables.ShapesFaceList(i + tableOffset, 2)];
    b[1] = ypoints[tables.ShapesFaceList(i + tableOffset, 2)];
    b[2] = zpoints[tables.ShapesFaceList(i + tableOffset, 2)];
    c[0] = xpoints[tables.ShapesFaceList(i + tableOffset, 3)];
    c[1] = ypoints[tables.ShapesFaceList(i + tableOffset, 3)];
    c[2] = zpoints[tables.ShapesFaceList(i + tableOffset, 3)];
    d[0] = xpoints[tables.ShapesFaceList(i + tableOffset, 4)];
    d[1] = ypoints[tables.ShapesFaceList(i + tableOffset, 4)];
    d[2] = zpoints[tables.ShapesFaceList(i + tableOffset, 4)];
    T distance = -1.f;
    distances[i] = distance; //init to -1

    T uNotUsed, vNotUsed;

    intersector.IntersectTriSn(a, b, c, s, k, distance, uNotUsed, vNotUsed, origin);

    if (distance != -1.f)
      distances[i] = distance;
    //
    //First two faces are triangles
    //
    if (i < 2)
      continue;
    distance = -1.f;

    intersector.IntersectTriSn(a, c, d, s, k, distance, uNotUsed, vNotUsed, origin);

    if (distance != -1.f)
    {
      if (distances[i] != -1.f)
        distances[i] = viskores::Min(distance, distances[i]);
      else
        distances[i] = distance;
    }
  }
}

//
// General Template should never be instantiated
//

template <int CellType>
class CellIntersector
{
public:
  template <typename T>
  VISKORES_EXEC_CONT inline void IntersectCell(
    T* viskoresNotUsed(xpoints),
    T* viskoresNotUsed(ypoints),
    T* viskoresNotUsed(zpoints),
    const viskores::Vec<T, 3>& viskoresNotUsed(dir),
    const viskores::Vec<T, 3>& viskoresNotUsed(origin),
    T* viskoresNotUsed(distances),
    const viskores::UInt8 viskoresNotUsed(cellShape = 12));
};

//
// Hex Specialization
//
template <>
class CellIntersector<CELL_SHAPE_HEXAHEDRON>
{
public:
  template <typename T>
  VISKORES_EXEC_CONT inline void IntersectCell(T* xpoints,
                                               T* ypoints,
                                               T* zpoints,
                                               const viskores::Vec<T, 3>& dir,
                                               const viskores::Vec<T, 3>& origin,
                                               T* distances,
                                               const viskores::UInt8 cellShape = 12) const
  {
    if (cellShape == 12)
    {
      IntersectZoo(xpoints, ypoints, zpoints, dir, origin, distances, cellShape);
    }
    else
    {
      printf("CellIntersector Hex Error: unsupported cell type. Doing nothing\n");
    }
  }
};
//
//
// Hex Specialization Structured
//
template <>
class CellIntersector<254>
{
public:
  template <typename T>
  VISKORES_EXEC_CONT inline void IntersectCell(T* xpoints,
                                               T* ypoints,
                                               T* zpoints,
                                               const viskores::Vec<T, 3>& dir,
                                               const viskores::Vec<T, 3>& origin,
                                               T* distances,
                                               const viskores::UInt8 cellShape = 12) const
  {
    if (cellShape == 12)
    {
      IntersectZoo(xpoints, ypoints, zpoints, dir, origin, distances, cellShape);
    }
    else
    {
      printf("CellIntersector Hex Error: unsupported cell type. Doing nothing\n");
    }
  }
};

//
// Tet Specialization
//
template <>
class CellIntersector<CELL_SHAPE_TETRA>
{
public:
  template <typename T>
  VISKORES_EXEC_CONT inline void IntersectCell(T* xpoints,
                                               T* ypoints,
                                               T* zpoints,
                                               const viskores::Vec<T, 3>& dir,
                                               const viskores::Vec<T, 3>& origin,
                                               T distances[6],
                                               const viskores::UInt8 cellShape = 12) const
  {
    if (cellShape == CELL_SHAPE_TETRA)
    {
      IntersectTet(xpoints, ypoints, zpoints, dir, origin, distances);
    }
    else
    {
      printf("CellIntersector Tet Error: unsupported cell type. Doing nothing\n");
    }
  }
};

//
// Wedge Specialization
//
template <>
class CellIntersector<CELL_SHAPE_WEDGE>
{
public:
  template <typename T>
  VISKORES_EXEC_CONT inline void IntersectCell(T* xpoints,
                                               T* ypoints,
                                               T* zpoints,
                                               const viskores::Vec<T, 3>& dir,
                                               const viskores::Vec<T, 3>& origin,
                                               T distances[6],
                                               const viskores::UInt8 cellShape = 12) const
  {
    if (cellShape == CELL_SHAPE_WEDGE)
    {
      IntersectWedge(xpoints, ypoints, zpoints, dir, origin, distances);
    }
    else
    {
      printf("CellIntersector Wedge Error: unsupported cell type. Doing nothing\n");
    }
  }
};
//
// Zoo elements
//
template <>
class CellIntersector<255>
{
public:
  template <typename T>
  VISKORES_EXEC_CONT inline void IntersectCell(T* xpoints,
                                               T* ypoints,
                                               T* zpoints,
                                               const viskores::Vec<T, 3>& dir,
                                               const viskores::Vec<T, 3>& origin,
                                               T distances[6],
                                               const viskores::UInt8 cellShape = 0) const
  {
    IntersectZoo(xpoints, ypoints, zpoints, dir, origin, distances, cellShape);
  }
};
}
}
} // namespace viskores::rendering::raytracing
#endif
