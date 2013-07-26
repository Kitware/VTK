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

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkGraphGeodesicPath.h"

class vtkDijkstraGraphInternals;
class vtkIdList;

class VTKFILTERSMODELING_EXPORT vtkDijkstraGraphGeodesicPath :
                           public vtkGraphGeodesicPath
{
public:

  // Description:
  // Instantiate the class
  static vtkDijkstraGraphGeodesicPath *New();

  // Description:
  // Standard methids for printing and determining type information.
  vtkTypeMacro(vtkDijkstraGraphGeodesicPath,vtkGraphGeodesicPath);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The vertex ids (of the input polydata) on the shortest path
  vtkGetObjectMacro(IdList, vtkIdList);

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
  // Use the input point to repel the path by assigning high costs.
  vtkSetMacro(RepelPathFromVertices, int);
  vtkGetMacro(RepelPathFromVertices, int);
  vtkBooleanMacro(RepelPathFromVertices, int);

  // Description:
  // Specify vtkPoints to use to repel the path from.
  virtual void SetRepelVertices(vtkPoints*);
  vtkGetObjectMacro(RepelVertices, vtkPoints);

  //Description:
  //Fill the array with the cumulative weights.
  virtual void GetCumulativeWeights(vtkDoubleArray *weights);

protected:
  vtkDijkstraGraphGeodesicPath();
  ~vtkDijkstraGraphGeodesicPath();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  // Build a graph description of the input.
  virtual void BuildAdjacency( vtkDataSet *inData );

  vtkTimeStamp AdjacencyBuildTime;

  // The fixed cost going from vertex u to v.
  virtual double CalculateStaticEdgeCost( vtkDataSet *inData, vtkIdType u, vtkIdType v);

  // The cost going from vertex u to v that may depend on one or more vertices
  //that precede u.
  virtual double CalculateDynamicEdgeCost( vtkDataSet *, vtkIdType , vtkIdType )
  { return 0.0; }

  void Initialize( vtkDataSet *inData );

  void Reset();

  // Calculate shortest path from vertex startv to vertex endv.
  virtual void ShortestPath( vtkDataSet *inData, int startv, int endv );

  // Relax edge u,v with weight w.
  void Relax(const int& u, const int& v, const double& w);

  // Backtrace the shortest path
  void TraceShortestPath( vtkDataSet* inData, vtkPolyData* outPoly,
               vtkIdType startv, vtkIdType endv);

  // The number of vertices.
  int NumberOfVertices;

  // The vertex ids on the shortest path.
  vtkIdList *IdList;

  //Internalized STL containers.
  vtkDijkstraGraphInternals *Internals;

  int StopWhenEndReached;
  int UseScalarWeights;
  int RepelPathFromVertices;

  vtkPoints* RepelVertices;

private:
  vtkDijkstraGraphGeodesicPath(const vtkDijkstraGraphGeodesicPath&);  // Not implemented.
  void operator=(const vtkDijkstraGraphGeodesicPath&);  // Not implemented.

};

#endif

