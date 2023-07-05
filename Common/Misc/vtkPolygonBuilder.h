// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolygonBuilder
 *
 *
 *  The polygon output is the boundary of the union of the triangles.
 *  It is assumed that the input triangles form a simple polygon. It is
 *  currently used to compute polygons for slicing.
 *
 */

#ifndef vtkPolygonBuilder_h
#define vtkPolygonBuilder_h

#include "vtkCommonMiscModule.h" // For export macro
#include "vtkIdList.h"
#include "vtkObject.h"
#include "vtkType.h" //for basic types
#include <cstddef>   //for size_t
#include <map>       //for private data members
#include <utility>   //for private data members
#include <vector>    // for private data members

VTK_ABI_NAMESPACE_BEGIN
class vtkIdListCollection;

class VTKCOMMONMISC_EXPORT vtkPolygonBuilder
{
public:
  vtkPolygonBuilder();

  /**
   * Insert a triangle as a triplet of point IDs.
   */
  void InsertTriangle(const vtkIdType* abc);

  /**
   * Populate polys with lists of polygons, defined as sequential external
   * vertices. It is the responsibility of the user to delete these generated
   * lists in order to avoid memory leaks.
   */
  void GetPolygons(vtkIdListCollection* polys);

  /**
   * Prepare the builder for a new set of inputs.
   */
  void Reset();

private:
  typedef std::pair<vtkIdType, vtkIdType> Edge;
  typedef std::map<Edge, size_t> EdgeHistogram;
  typedef std::multimap<vtkIdType, vtkIdType> EdgeMap;
  typedef std::vector<vtkIdType> Triangle;
  typedef std::vector<Triangle> Triangles;
  typedef std::map<vtkIdType, Triangles> TriangleMap;

  TriangleMap Tris;

  EdgeHistogram EdgeCounter;
  EdgeMap Edges;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkPolygonBuilder.h
