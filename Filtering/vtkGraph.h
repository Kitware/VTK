/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraph.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkGraph - Base class for graph data types.
//
// .SECTION Description
// vtkGraph is the abstract base class that provides all read-only API for graph
// data types. A graph consists of a collection of vertices and a
// collection of edges connecting pairs of vertices. The vtkDirectedGraph
// subclass represents a graph whose edges have inherent order from source
// vertex to target vertex, while vtkUndirectedGraph is a graph whose edges
// have no inherent ordering.
//
// Graph vertices may be traversed in two ways. In the current implementation,
// all vertices are assigned consecutive ids starting at zero, so they may
// be traversed in a simple for loop from 0 to graph->GetNumberOfVertices() - 1.
// You may alternately create a vtkVertexListIterator and call graph->GetVertices(it).
// it->Next() will return the id of the next vertex, while it->HasNext() indicates
// whether there are more vertices in the graph.
// This is the preferred method, since in the future graphs may support filtering
// or subsetting where the vertex ids may not be contiguous.
//
// Graph edges must be traversed through iterators. To traverse all edges
// in a graph, create an instance of vtkEdgeListIterator and call graph->GetEdges(it).
// it->Next() returns lightweight vtkEdgeType structures, which contain the public
// fields Id, Source and Target. Id is the identifier for the edge, which may
// be used to look up values in assiciated edge data arrays. Source and Target
// store the ids of the source and target vertices of the edge. Note that the
// edge list iterator DOES NOT necessarily iterate over edges in order of ascending
// id. To traverse edges from wrapper code (Python, Tcl, Java), use
// it->NextGraphEdge() instead of it->Next().  This will return a heavyweight,
// wrappable vtkGraphEdge object, which has the same fields as vtkEdgeType
// accessible through getter methods.
//
// To traverse all edges outgoing from a vertex, create a vtkOutEdgeIterator and 
// call graph->GetOutEdges(v, it). it->Next() returns a lightweight vtkOutEdgeType
// containing the fields Id and Target. The source of the edge is always the
// vertex that was passed as an argument to GetOutEdges().
// Incoming edges may be similarly traversed with vtkInEdgeIterator, which returns
// vtkInEdgeType structures with Id and Source fields.
// Both vtkOutEdgeIterator and vtkInEdgeIterator also provide the wrapper functions
// NextGraphEdge() which return vtkGraphEdge objects.
//
// An additional iterator, vtkAdjacentVertexIterator can traverse outgoing vertices
// directly, instead needing to parse through edges. Initialize the iterator by
// calling graph->GetAdjacentVertices(v, it).
//
// vtkGraph has two instances of vtkDataSetAttributes for associated
// vertex and edge data. It also has a vtkPoints instance which may store
// x,y,z locations for each vertex. This is populated by filters such as
// vtkGraphLayout and vtkAssignCoordinates.
//
// All graph types share the same implementation, so the structure of one
// may be shared among multiple graphs, even graphs of different types.
// Structures from vtkUndirectedGraph and vtkMutableUndirectedGraph may be
// shared directly.  Structures from vtkDirectedGraph, vtkMutableDirectedGraph,
// and vtkTree may be shared directly with the exception that setting a
// structure to a tree requires that a "is a tree" test passes.
//
// For graph types that are known to be compatible, calling ShallowCopy()
// or DeepCopy() will work as expected.  When the outcome of a conversion
// is unknown (i.e. setting a graph to a tree), CheckedShallowCopy() and 
// CheckedDeepCopy() exist which are identical to ShallowCopy() and DeepCopy(), 
// except that instead of emitting an error for an incompatible structure,
// the function returns false.  This allows you to programmatically check
// structure compatibility without causing error messages.
//
// To construct a graph, use vtkMutableDirectedGraph or 
// vtkMutableUndirectedGraph. You may then use CheckedShallowCopy
// to set the contents of a mutable graph type into one of the non-mutable
// types vtkDirectedGraph, vtkUndirectedGraph.
// To construct a tree, use vtkMutableDirectedGraph, with directed edges
// which point from the parent to the child, then use CheckedShallowCopy
// to set the structure to a vtkTree.
//
// .SECTION Caveats
// All copy operations implement copy-on-write. The structures are initially
// shared, but if one of the graphs is modified, the structure is copied
// so that to the user they function as if they were deep copied. This means
// that care must be taken if different threads are accessing different graph
// instances that share the same structure. Race conditions may develop if
// one thread is modifying the graph at the same time that another graph is
// copying the structure.
//
// .SECTION See Also
// vtkDirectedGraph vtkUndirectedGraph vtkMutableDirectedGraph 
// vtkMutableUndirectedGraph vtkTree
// 
// .SECTION Thanks
// Thanks to Brian Wylie, Timothy Shead, Ken Moreland of Sandia National
// Laboratories and Douglas Gregor of Indiana University for designing these
// classes.

#ifndef __vtkGraph_h
#define __vtkGraph_h

#include "vtkDataObject.h"

class vtkAdjacentVertexIterator;
class vtkEdgeListIterator;
class vtkDataSetAttributes;
class vtkGraphInternals;
class vtkIdTypeArray;
class vtkInEdgeIterator;
class vtkOutEdgeIterator;
class vtkPoints;
class vtkVertexListIterator;

//BTX
// Forward declare some boost stuff even if boost wrappers
// are turned off.
namespace boost
{
  class vtk_edge_iterator;
  class vtk_out_edge_pointer_iterator;
  class vtk_in_edge_pointer_iterator;
}

// Edge structures.
struct vtkEdgeBase
{
  vtkEdgeBase() { }
  vtkEdgeBase(vtkIdType id) :
    Id(id) { }
  vtkIdType Id;
};

struct vtkOutEdgeType : vtkEdgeBase
{
  vtkOutEdgeType() { }
  vtkOutEdgeType(vtkIdType t, vtkIdType id) :
    vtkEdgeBase(id),
    Target(t) { }
  vtkIdType Target;
};

struct vtkInEdgeType : vtkEdgeBase
{
  vtkInEdgeType() { }
  vtkInEdgeType(vtkIdType s, vtkIdType id) :
    vtkEdgeBase(id),
    Source(s) { }
  vtkIdType Source;
};

struct vtkEdgeType : vtkEdgeBase
{
  vtkEdgeType() { }
  vtkEdgeType(vtkIdType s, vtkIdType t, vtkIdType id) :
    vtkEdgeBase(id),
    Source(s),
    Target(t) { }
  vtkIdType Source;
  vtkIdType Target;
};
//ETX

class VTK_FILTERING_EXPORT vtkGraph : public vtkDataObject
{
public:
  vtkTypeRevisionMacro(vtkGraph, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the vertex or edge data.
  vtkGetObjectMacro(VertexData, vtkDataSetAttributes);
  vtkGetObjectMacro(EdgeData, vtkDataSetAttributes);

  // Description:
  // Return what type of dataset this is.
  virtual int GetDataObjectType() {return VTK_GRAPH;}

  // Description:
  // Initialize to an empty graph.
  virtual void Initialize();

  // Description:
  // These methods return the point (0,0,0) until the points structure
  // is created, when it returns the actual point position.
  double *GetPoint(vtkIdType ptId);
  void GetPoint(vtkIdType ptId, double x[3]);

  // Description:
  // Returns the points array for this graph.
  // If points is not yet constructed, generates and returns 
  // a new points array filled with (0,0,0) coordinates.
  vtkPoints* GetPoints();
  virtual void SetPoints(vtkPoints *points);

  // Description:
  // Compute the bounds of the graph.
  void ComputeBounds();

  // Description:
  // Return a pointer to the geometry bounding box in the form
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  double *GetBounds();
  void GetBounds(double bounds[6]);

  // Description:
  // The modified time of the graph.
  unsigned long int GetMTime();

  // Description:
  // Initializes the out edge iterator to iterate over
  // all outgoing edges of vertex v.  For an undirected graph,
  // returns all incident edges.
  virtual void GetOutEdges(vtkIdType v, vtkOutEdgeIterator *it);
  
  // Description:
  // The total of all incoming and outgoing vertices for vertex v.
  // For undirected graphs, this is simply the number of edges incident
  // to v.
  virtual vtkIdType GetDegree(vtkIdType v);

  // Description:
  // The number of outgoing edges from vertex v.
  // For undirected graphs, returns the same as GetDegree().
  virtual vtkIdType GetOutDegree(vtkIdType v);
  
  // Description:
  // Initializes the in edge iterator to iterate over
  // all incoming edges to vertex v.  For an undirected graph,
  // returns all incident edges.
  virtual void GetInEdges(vtkIdType v, vtkInEdgeIterator *it);
  
  // Description:
  // The number of incoming edges to vertex v.
  // For undirected graphs, returns the same as GetDegree().
  virtual vtkIdType GetInDegree(vtkIdType v);

  // Description:
  // Initializes the adjacent vertex iterator to iterate over
  // all outgoing vertices from vertex v.  For an undirected graph,
  // returns all adjacent vertices.
  virtual void GetAdjacentVertices(vtkIdType v, vtkAdjacentVertexIterator *it);

  // Description:
  // Initializes the edge list iterator to iterate over all
  // edges in the graph. Edges may not be traversed in order of 
  // increasing edge id.
  virtual void GetEdges(vtkEdgeListIterator *it);
  
  // Description:
  // The number of edges in the graph.
  virtual vtkIdType GetNumberOfEdges();

  // Description:
  // Initializes the vertex list iterator to iterate over all
  // vertices in the graph.
  virtual void GetVertices(vtkVertexListIterator *it);
  
  // Description:
  // The number of vertices in the graph.
  virtual vtkIdType GetNumberOfVertices();

  // Description:
  // Shallow copies the data object into this graph.
  // If it is an incompatible graph, reports an error.
  virtual void ShallowCopy(vtkDataObject *obj);

  // Description:
  // Deep copies the data object into this graph.
  // If it is an incompatible graph, reports an error.
  virtual void DeepCopy(vtkDataObject *obj);

  // Description:
  // Performs the same operation as ShallowCopy(),
  // but instead of reporting an error for an incompatible graph,
  // returns false.
  virtual bool CheckedShallowCopy(vtkGraph *g);
  
  // Description:
  // Performs the same operation as DeepCopy(),
  // but instead of reporting an error for an incompatible graph,
  // returns false.
  virtual bool CheckedDeepCopy(vtkGraph *g);

  // Decription:
  // Reclaim unused memory.
  virtual void Squeeze();

  //BTX
  // Description:
  // Retrieve a graph from an information vector.
  static vtkGraph *GetData(vtkInformation *info);
  static vtkGraph *GetData(vtkInformationVector *v, int i=0);
  //ETX

  // Description:
  // Reorder the outgoing vertices of a vertex.
  // The vertex list must have the same elements as the current out edge
  // list, just in a different order.
  // This method does not change the topology of the graph.
  void ReorderOutVertices(vtkIdType v, vtkIdTypeArray *vertices);

  // Description:
  // Returns true if both graphs point to the same adjacency structure.
  // Can be used to test the copy-on-write feature of the graph.
  bool IsSameStructure(vtkGraph *other);

protected:
  //BTX
  vtkGraph();
  ~vtkGraph();

  // Description:
  // Protected method for adding vertices
  // used by mutable subclasses.
  vtkIdType AddVertexInternal();

  // Description:
  // Protected method for adding edges of a certain directedness
  // used by mutable subclasses.
  vtkEdgeType AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed);

  // Description:
  // Subclasses override this method to accept the structure
  // based on their requirements.
  virtual bool IsStructureValid(vtkGraph *g) = 0;

  // Description:
  // Copy internal data structure.
  virtual void CopyInternal(vtkGraph *g, bool deep);

  // Description:
  // The adjacency list internals of this graph.
  vtkGraphInternals *Internals;

  // Description:
  // Private method for setting internals.
  void SetInternals(vtkGraphInternals* internals);

  // Description:
  // If this instance does not own its internals, it makes a copy of the
  // internals.  This is called before any write operation.
  void ForceOwnership();

  // Description:
  // Fast access functions for iterators.
  virtual void GetOutEdges(vtkIdType v, const vtkOutEdgeType *& edges, vtkIdType & nedges);
  virtual void GetInEdges(vtkIdType v, const vtkInEdgeType *& edges, vtkIdType & nedges);

  // Description:
  // Friend iterator classes.
  friend class vtkAdjacentVertexIterator;
  friend class vtkEdgeListIterator;
  friend class vtkInEdgeIterator;
  friend class vtkOutEdgeIterator;
  friend class boost::vtk_edge_iterator;
  friend class boost::vtk_in_edge_pointer_iterator;
  friend class boost::vtk_out_edge_pointer_iterator;

  // Description:
  // The vertex and edge data.
  vtkDataSetAttributes *VertexData;
  vtkDataSetAttributes *EdgeData;

  // Description:
  // (xmin,xmax, ymin,ymax, zmin,zmax) geometric bounds.
  double Bounds[6];

  // Description:
  // Time at which bounds were computed.
  vtkTimeStamp ComputeTime; 

  // Description:
  // The vertex locations.
  vtkPoints *Points;
  static double DefaultPoint[3];
  //ETX
private:
  vtkGraph(const vtkGraph&);  // Not implemented.
  void operator=(const vtkGraph&);  // Not implemented.
};

//BTX
bool VTK_FILTERING_EXPORT operator==(vtkEdgeBase e1, vtkEdgeBase e2);
bool VTK_FILTERING_EXPORT operator!=(vtkEdgeBase e1, vtkEdgeBase e2);
VTK_FILTERING_EXPORT ostream& operator<<(ostream& out, vtkEdgeBase e);
//ETX

#endif
