/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonBuilder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolygonBuilder -builds a polygon from a set of abstract triangles (represented by index triplets)
//
// .SECTION Description
//  The polygon output is the boundary of the union of the triangles.
//  It is assumed that the input triangles form a simple polygon. It is
//  currently used to compute polygons for slicing.
//

#ifndef vtkPolygonBuilder_h
#define vtkPolygonBuilder_h

#include "vtkCommonMiscModule.h" // For export macro
#include <map> //for private data members
#include <utility> //for private data members
#include "vtkType.h" //for basic types
#include <cstddef> //for size_t
#include "vtkObject.h"

class vtkIdListCollection;

class VTKCOMMONMISC_EXPORT vtkPolygonBuilder
{
public:
  vtkPolygonBuilder();

  // Description:
  // Insert a triangle as a triplet of point IDs.
  void InsertTriangle(vtkIdType* abc);

  // Description:
  // Populate polys with lists of polygons, defined as sequential external
  // vertices. It is the responsibility of the user to delete these generated
  // lists in order to avoid memory leaks.
  void GetPolygons(vtkIdListCollection* polys);

  // Description:
  // Prepare the builder for a new set of inputs.
  void Reset();

private:
  typedef std::pair<vtkIdType,vtkIdType> Edge;
  typedef std::map<Edge,size_t> EdgeHistogram;
  typedef std::multimap<vtkIdType,vtkIdType> EdgeMap;

  EdgeHistogram EdgeCounter;
  EdgeMap Edges;
};

#endif
// VTK-HeaderTest-Exclude: vtkPolygonBuilder.h
