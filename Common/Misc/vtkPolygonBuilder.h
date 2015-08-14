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
class vtkIdList;

class VTKCOMMONMISC_EXPORT vtkPolygonBuilder
{
public:
  vtkPolygonBuilder();
  void InsertTriangle(vtkIdType* abc);
  void GetPolygon(vtkIdList* poly);
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
