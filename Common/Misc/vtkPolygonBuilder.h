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
//  It is assumed that the input triangles form a simple polygon without
//  internal vertices.
//  The algorithm is quadratic to the input size, but
//  can be sped up easily by improving the FindEdge() function. It is
//  currently used to compute polygon for slicing.
//

#ifndef vtkPolygonBuilder_h
#define vtkPolygonBuilder_h

#include "vtkCommonMiscModule.h" // For export macro
#include <vector> //for private data members
#include "vtkType.h" //for basic types
#include <cstddef> //for size_t
#include "vtkObject.h"
class vtkIdList;

class VTKCOMMONMISC_EXPORT vtkPolygonBuilder
{
public:
  vtkPolygonBuilder();
  void Reset();
  bool InsertTriangle(vtkIdType* abc);
  void GetPolygon(vtkIdList* poly) const;

private:
  typedef size_t VertexRef;
  struct Vertex
  {
    VertexRef Next;
    vtkIdType Vert;
    Vertex(VertexRef next, vtkIdType vert):Next(next), Vert(vert){}
  };

  bool FindEdge(vtkIdType a, vtkIdType b, VertexRef& v) const;
  VertexRef Insert(VertexRef i, vtkIdType vertexId);
  vtkIdType GetVertexId(VertexRef i) const { return this->Poly[i].Vert;}
  VertexRef GetNextVertex(VertexRef i) const { return this->Poly[i].Next;}

  std::vector<Vertex> Poly;
};

#endif
// VTK-HeaderTest-Exclude: vtkPolygonBuilder.h
