/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDijkstraGraphInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDijkstraGraphInternals
// .SECTION Description

#ifndef __vtkDijkstraGraphInternals_h
#define __vtkDijkstraGraphInternals_h

#include <vtkstd/vector>
#include <vtkstd/map>

//-----------------------------------------------------------------------------
class vtkDijkstraGraphInternals
{
public:

  // CumulativeWeights(v) current summed weight for path to vertex v.
  vtkstd::vector<double> CumulativeWeights;
  
  // Predecessors(v) predecessor of v.
  vtkstd::vector<int> Predecessors;
  
  // OpenVertices is the set of vertices wich has not a shortest path yet but has a path.
  // OpenVertices(v) == 1 means that vertex v is in OpenVertices.
  // OpenVertices is a boolean (1/0) array.
  vtkstd::vector<bool> OpenVertices;
  
  // ClosedVertices is the set of vertices with already determined shortest path
  // ClosedVertices(v) == 1 means that vertex v is in ClosedVertices.
  // ClosedVertices is a boolean (1/0) array.
  vtkstd::vector<bool> ClosedVertices;
  
  // The priority que (a binary heap) with vertex indices.
  vtkstd::vector<int> Heap;
  
  // HeapIndices(v) the position of v in Heap (HeapIndices and Heap are kind of inverses).
  vtkstd::vector<int> HeapIndices;

  // Adjacency representation.
  vtkstd::vector<vtkstd::map<int,double>> Adjacency;

  // Path repelling by assigning high costs to flagged vertices.
  vtkstd::vector<bool> BlockedVertices;

};

#endif
