/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDijkstraGraphGeodesicPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDijkstraGraphGeodesicPath - Dijkstra algorithm to compute the graph geodesic.
// .SECTION Description
// Takes as input a polygonal mesh and performs a single source shortest 
// path calculation. Dijkstra's algorithm is used. The implementation is 
// similar to the one described in Introduction to Algorithms (Second Edition)
// by Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, and 
// Cliff Stein, published by MIT Press and McGraw-Hill. Some minor 
// enhancement are added though. All vertices are not pushed on the heap
// at start, instead a front set is maintained. The heap is implemented as 
// a binary heap. The output of the filter is a set of lines describing 
// the shortest path from StartVertex to EndVertex.
//
// .SECTION Caveats
// The input polydata must have only triangle cells. 
//
// .SECTION Thanks
// The class was contributed by Rasmus Paulsen. 
// www.imm.dtu.dk/~rrp/VTK . Also thanks to Alexandre Gouaillard and Shoaib 
// Ghias for bug fixes and enhancements.

#ifndef __vtkDijkstraGraphGeodesicPath_h
#define __vtkDijkstraGraphGeodesicPath_h

#include "vtkGraphGeodesicPath.h"

class vtkDoubleArray;
class vtkIntArray;
class vtkIdList;
class vtkFloatArray;

class VTK_GRAPHICS_EXPORT vtkDijkstraGraphGeodesicPath :
                           public vtkGraphGeodesicPath
{
public:
  
  // Description:
  // Instantiate the class
  static vtkDijkstraGraphGeodesicPath *New();

  // Description:
  // Standard methids for printing and determining type information.
  vtkTypeRevisionMacro(vtkDijkstraGraphGeodesicPath,vtkGraphGeodesicPath);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The vertex ids (of the input polydata) on the shortest path
  vtkGetObjectMacro(IdList, vtkIdList);
  
  // Description:
  // Get the summed weight for all vertices
  vtkGetObjectMacro(d, vtkFloatArray);
  
  // Description: 
  // Stop when the end vertex is reached 
  // or calculate shortest path to all vertices
  vtkSetMacro(StopWhenEndReached, int);
  vtkGetMacro(StopWhenEndReached, int);
  vtkBooleanMacro(StopWhenEndReached, int);
  
  // Description: 
  // Use scalar values in the edge weight (experimental)
  vtkSetMacro(UseScalarWeights, int);
  vtkGetMacro(UseScalarWeights, int);
  vtkBooleanMacro(UseScalarWeights, int);

  // Description:
  // TODO: Get the total geodesic length.
  virtual double GetGeodesicLength() { return 0.0; }

protected:
  vtkDijkstraGraphGeodesicPath();
  ~vtkDijkstraGraphGeodesicPath();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, 
                          vtkInformationVector *);

  // Build a graph description of the mesh
  void BuildAdjacency(vtkPolyData *pd);

  void DeleteAdjacency();

  vtkTimeStamp AdjacencyBuildTime;
  
  // The cost going from vertex u to v
  // TODO: should be implemented as a user supplied
  // callback function
  double CalculateEdgeCost(vtkPolyData *pd, vtkIdType u, vtkIdType v);

  void Initialize();

  void Reset();

  // structure the heap
  void Heapify(int i);
  
  // insert vertex v in heap. Weight is in d(v)
  void HeapInsert(int v);
  
  // extract vertex with min d(v)
  int HeapExtractMin();
  
  // Update heap when key d(v) has been decreased
  void HeapDecreaseKey(int v);
  
  void InitSingleSource(int startv);
  
  // Calculate shortest path from vertex startv to vertex endv
  void ShortestPath(int startv, int endv);
  
  // Relax edge u,v with weight w
  void Relax(int u, int v, double w);

  // Backtrace the shortest path
  void TraceShortestPath(vtkPolyData *inPd, vtkPolyData *outPd,
               vtkIdType startv, vtkIdType endv);
  
  // the number of vertices
  int NumberOfVertices;
  
  // d(v) current summed weight for path to vertex v
  vtkFloatArray *d;
  
  // pre(v) predecessor of v
  vtkIntArray *pre;
  
  // f is the set of vertices wich has not a shortest path yet but has a path
  // ie. the front set (f(v) == 1 means that vertex v is in f)
  vtkIntArray *f;
  
  // s is the set of vertices with already determined shortest path
  // s(v) == 1 means that vertex v is in s
  vtkIntArray *s;
  
  // the priority que (a binary heap) with vertex indices
  vtkIntArray *Heap;
  
  // The real number of elements in H != H.size()
  int HeapSize;
  
  // p(v) the position of v in H (p and H are kind of inverses)
  vtkIntArray *p;
  
  // The vertex ids on the shortest path
  vtkIdList *IdList;
  
  // Adjacency representation
  vtkIdList **Adjacency;
  
  int StopWhenEndReached;
  
  int UseScalarWeights;

  // Used to remember the size of the graph. If the filter is re-used.
  int AdjacencyGraphSize;

private:
  vtkDijkstraGraphGeodesicPath(const vtkDijkstraGraphGeodesicPath&);  // Not implemented.
  void operator=(const vtkDijkstraGraphGeodesicPath&);  // Not implemented.

};

#endif

