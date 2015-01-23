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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
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
// .SECTION Vertex pedigree IDs
// The vertices in a vtkGraph can be associated with pedigree IDs
// through GetVertexData()->SetPedigreeIds. In this case, there is a
// 1-1 mapping between pedigree Ids and vertices. One can query the
// vertex ID based on the pedigree ID using FindVertex, add new
// vertices by pedigree ID with AddVertex, and add edges based on the
// pedigree IDs of the source and target vertices. For example,
// AddEdge("Here", "There") will find (or add) vertices with pedigree
// ID "Here" and "There" and then introduce an edge from "Here" to
// "There".
//
// To configure the vtkGraph with a pedigree ID mapping, create a
// vtkDataArray that will store the pedigree IDs and set that array as
// the pedigree ID array for the vertices via
// GetVertexData()->SetPedigreeIds().
//
// .SECTION Distributed graphs
//
// vtkGraph instances can be distributed across multiple machines, to
// allow the construction and manipulation of graphs larger than a
// single machine could handle. A distributed graph will typically be
// distributed across many different nodes within a cluster, using the
// Message Passing Interface (MPI) to allow those cluster nodes to
// communicate.
//
// An empty vtkGraph can be made into a distributed graph by attaching
// an instance of a vtkDistributedGraphHelper via the
// SetDistributedGraphHelper() method. To determine whether a graph is
// distributed or not, call GetDistributedGraphHelper() and check
// whether the result is non-NULL. For a distributed graph, the number
// of processors across which the graph is distributed can be
// retrieved by extracting the value for the DATA_NUMBER_OF_PIECES key
// in the vtkInformation object (retrieved by GetInformation())
// associated with the graph. Similarly, the value corresponding to
// the DATA_PIECE_NUMBER key of the vtkInformation object describes
// which piece of the data this graph instance provides.
//
// Distributed graphs behave somewhat differently from non-distributed
// graphs, and will require special care. In a distributed graph, each
// of the processors will contain a subset of the vertices in the
// graph. That subset of vertices can be accessed via the
// vtkVertexListIterator produced by GetVertices().
// GetNumberOfVertices(), therefore, returns the number of vertices
// stored locally: it does not account for vertices stored on other
// processors. A vertex (or edge) is identified by both the rank of
// its owning processor and by its index within that processor, both
// of which are encoded within the vtkIdType value that describes that
// vertex (or edge). The owning processor is a value between 0 and
// P-1, where P is the number of processors across which the vtkGraph
// has been distributed. The local index will be a value between 0 and
// GetNumberOfVertices(), for vertices, or GetNumberOfEdges(), for
// edges, and can be used to access the local parts of distributed
// data arrays. When given a vtkIdType identifying a vertex, one can
// determine the owner of the vertex with
// vtkDistributedGraphHelper::GetVertexOwner() and the local index
// with vtkDistributedGraphHelper::GetVertexIndex(). With edges, the
// appropriate methods are vtkDistributedGraphHelper::GetEdgeOwner()
// and vtkDistributedGraphHelper::GetEdgeIndex(), respectively. To
// construct a vtkIdType representing either a vertex or edge given
// only its owner and local index, use
// vtkDistributedGraphHelper::MakeDistributedId().
//
// The edges in a distributed graph are always stored on the
// processors that own the vertices named by the edge. For example,
// given a directed edge (u, v), the edge will be stored in the
// out-edges list for vertex u on the processor that owns u, and in
// the in-edges list for vertex v on the processor that owns v. This
// "row-wise" decomposition of the graph means that, for any vertex
// that is local to a processor, that processor can look at all of the
// incoming and outgoing edges of the graph. Processors cannot,
// however, access the incoming or outgoing edge lists of vertex owned
// by other processors. Vertices owned by other processors will not be
// encountered when traversing the vertex list via GetVertices(), but
// may be encountered by traversing the in- and out-edge lists of
// local vertices or the edge list.
//
// Distributed graphs can have pedigree IDs for the vertices in the
// same way that non-distributed graphs can. In this case, the
// distribution of the vertices in the graph is based on pedigree
// ID. For example, a vertex with the pedigree ID "Here" might land on
// processor 0 while a vertex pedigree ID "There" would end up on
// processor 3. By default, the pedigree IDs themselves are hashed to
// give a random (and, hopefully, even) distribution of the
// vertices. However, one can provide a different vertex distribution
// function by calling
// vtkDistributedGraphHelper::SetVertexPedigreeIdDistribution.  Once a
// distributed graph has pedigree IDs, the no-argument AddVertex()
// method can no longer be used. Additionally, once a vertex has a
// pedigree ID, that pedigree ID should not be changed unless the user
// can guarantee that the vertex distribution will still map that
// vertex to the same processor where it already resides.

// .SECTION See Also
// vtkDirectedGraph vtkUndirectedGraph vtkMutableDirectedGraph
// vtkMutableUndirectedGraph vtkTree vtkDistributedGraphHelper
//
// .SECTION Thanks
// Thanks to Brian Wylie, Timothy Shead, Ken Moreland of Sandia National
// Laboratories and Douglas Gregor of Indiana University for designing these
// classes.

#ifndef vtkGraph_h
#define vtkGraph_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"

class vtkAdjacentVertexIterator;
class vtkCellArray;
class vtkEdgeListIterator;
class vtkDataSetAttributes;
class vtkDirectedGraph;
class vtkGraphEdge;
class vtkGraphEdgePoints;
class vtkDistributedGraphHelper;
class vtkGraphInternals;
class vtkIdTypeArray;
class vtkInEdgeIterator;
class vtkOutEdgeIterator;
class vtkPoints;
class vtkUndirectedGraph;
class vtkVertexListIterator;
class vtkVariant;
class vtkVariantArray;

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

class VTKCOMMONDATAMODEL_EXPORT vtkGraph : public vtkDataObject
{
public:
  vtkTypeMacro(vtkGraph, vtkDataObject);
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
  // is created, when it returns the actual point position. In a
  // distributed graph, only the points for local vertices can be
  // retrieved.
  double *GetPoint(vtkIdType ptId);
  void GetPoint(vtkIdType ptId, double x[3]);

  // Description:
  // Returns the points array for this graph.
  // If points is not yet constructed, generates and returns
  // a new points array filled with (0,0,0) coordinates. In a
  // distributed graph, only the points for local vertices can be
  // retrieved or modified.
  vtkPoints* GetPoints();
  virtual void SetPoints(vtkPoints *points);

  // Description:
  // Compute the bounds of the graph. In a distributed graph, this
  // computes the bounds around the local part of the graph.
  void ComputeBounds();

  // Description:
  // Return a pointer to the geometry bounding box in the form
  // (xmin,xmax, ymin,ymax, zmin,zmax). In a distributed graph, this
  // computes the bounds around the local part of the graph.
  double *GetBounds();
  void GetBounds(double bounds[6]);

  // Description:
  // The modified time of the graph.
  unsigned long int GetMTime();

  // Description:
  // Initializes the out edge iterator to iterate over
  // all outgoing edges of vertex v.  For an undirected graph,
  // returns all incident edges. In a distributed graph, the vertex
  // v must be local to this processor.
  virtual void GetOutEdges(vtkIdType v, vtkOutEdgeIterator *it);

  // Description:
  // The total of all incoming and outgoing vertices for vertex v.
  // For undirected graphs, this is simply the number of edges incident
  // to v. In a distributed graph, the vertex v must be local to this
  // processor.
  virtual vtkIdType GetDegree(vtkIdType v);

  // Description:
  // The number of outgoing edges from vertex v.
  // For undirected graphs, returns the same as GetDegree(). In a
  // distributed graph, the vertex v must be local to this processor.
  virtual vtkIdType GetOutDegree(vtkIdType v);

  //BTX
  // Description:
  // Random-access method for retrieving outgoing edges from vertex v.
  virtual vtkOutEdgeType GetOutEdge(vtkIdType v, vtkIdType index);
  //ETX

  // Description:
  // Random-access method for retrieving outgoing edges from vertex v.
  // The method fills the vtkGraphEdge instance with the id, source, and
  // target of the edge. This method is provided for wrappers,
  // GetOutEdge(vtkIdType, vtkIdType) is preferred.
  virtual void GetOutEdge(vtkIdType v, vtkIdType index, vtkGraphEdge* e);

  // Description:
  // Initializes the in edge iterator to iterate over
  // all incoming edges to vertex v.  For an undirected graph,
  // returns all incident edges. In a distributed graph, the vertex
  // v must be local to this processor.
  virtual void GetInEdges(vtkIdType v, vtkInEdgeIterator *it);

  // Description:
  // The number of incoming edges to vertex v.
  // For undirected graphs, returns the same as GetDegree(). In a
  // distributed graph, the vertex v must be local to this processor.
  virtual vtkIdType GetInDegree(vtkIdType v);

  //BTX
  // Description:
  // Random-access method for retrieving incoming edges to vertex v.
  virtual vtkInEdgeType GetInEdge(vtkIdType v, vtkIdType index);
  //ETX

  // Description:
  // Random-access method for retrieving incoming edges to vertex v.
  // The method fills the vtkGraphEdge instance with the id, source, and
  // target of the edge. This method is provided for wrappers,
  // GetInEdge(vtkIdType, vtkIdType) is preferred.
  virtual void GetInEdge(vtkIdType v, vtkIdType index, vtkGraphEdge* e);

  // Description:
  // Initializes the adjacent vertex iterator to iterate over
  // all outgoing vertices from vertex v.  For an undirected graph,
  // returns all adjacent vertices. In a distributed graph, the vertex
  // v must be local to this processor.
  virtual void GetAdjacentVertices(vtkIdType v, vtkAdjacentVertexIterator *it);

  // Description:
  // Initializes the edge list iterator to iterate over all
  // edges in the graph. Edges may not be traversed in order of
  // increasing edge id. In a distributed graph, this returns edges
  // that are stored locally.
  virtual void GetEdges(vtkEdgeListIterator *it);

  // Description:
  // The number of edges in the graph. In a distributed graph,
  // this returns the number of edges stored locally.
  virtual vtkIdType GetNumberOfEdges();

  // Description:
  // Initializes the vertex list iterator to iterate over all
  // vertices in the graph. In a distributed graph, the iterator
  // traverses all local vertices.
  virtual void GetVertices(vtkVertexListIterator *it);

  // Description:
  // The number of vertices in the graph. In a distributed graph,
  // returns the number of local vertices in the graph.
  virtual vtkIdType GetNumberOfVertices();

  // BTX
  // Description:
  // Sets the distributed graph helper of this graph, turning it into a
  // distributed graph. This operation can only be executed on an empty
  // graph.
  void SetDistributedGraphHelper(vtkDistributedGraphHelper *helper);

  // Description:
  // Retrieves the distributed graph helper for this graph
  vtkDistributedGraphHelper *GetDistributedGraphHelper();
  //ETX

  // Description:
  // Retrieve the vertex with the given pedigree ID. If successful,
  // returns the ID of the vertex. Otherwise, either the vertex data
  // does not have a pedigree ID array or there is no vertex with the
  // given pedigree ID, so this function returns -1.
  // If the graph is a distributed graph, this method will return the
  // Distributed-ID of the vertex.
  vtkIdType FindVertex(const vtkVariant& pedigreeID);

  // Description:
  // Shallow copies the data object into this graph.
  // If it is an incompatible graph, reports an error.
  virtual void ShallowCopy(vtkDataObject *obj);

  // Description:
  // Deep copies the data object into this graph.
  // If it is an incompatible graph, reports an error.
  virtual void DeepCopy(vtkDataObject *obj);

  // Description:
  // Does a shallow copy of the topological information,
  // but not the associated attributes.
  virtual void CopyStructure(vtkGraph *g);

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

  // Description:
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
  // In a distributed graph, the vertex v must be local.
  void ReorderOutVertices(vtkIdType v, vtkIdTypeArray *vertices);

  // Description:
  // Returns true if both graphs point to the same adjacency structure.
  // Can be used to test the copy-on-write feature of the graph.
  bool IsSameStructure(vtkGraph *other);

  // Description:
  // Retrieve the source and target vertices for an edge id.
  // NOTE: The first time this is called, the graph will build
  // a mapping array from edge id to source/target that is the
  // same size as the number of edges in the graph. If you have
  // access to a vtkOutEdgeType, vtkInEdgeType, vtkEdgeType, or
  // vtkGraphEdge, you should directly use these structures
  // to look up the source or target instead of this method.
  vtkIdType GetSourceVertex(vtkIdType e);
  vtkIdType GetTargetVertex(vtkIdType e);

  //BTX
  // Description:
  // Get/Set the internal edge control points associated with each edge.
  // The size of the pts array is 3*npts, and holds the x,y,z
  // location of each edge control point.
  void SetEdgePoints(vtkIdType e, vtkIdType npts, double* pts);
  void GetEdgePoints(vtkIdType e, vtkIdType& npts, double*& pts);
  //ETX

  // Description:
  // Get the number of edge points associated with an edge.
  vtkIdType GetNumberOfEdgePoints(vtkIdType e);

  // Description:
  // Get the x,y,z location of a point along edge e.
  double* GetEdgePoint(vtkIdType e, vtkIdType i);

  // Description:
  // Clear all points associated with an edge.
  void ClearEdgePoints(vtkIdType e);

  // Description:
  // Set an x,y,z location of a point along an edge.
  // This assumes there is already a point at location i, and simply
  // overwrites it.
  void SetEdgePoint(vtkIdType e, vtkIdType i, double x[3]);
  void SetEdgePoint(vtkIdType e, vtkIdType i, double x, double y, double z)
    { double p[3] = {x, y, z}; this->SetEdgePoint(e, i, p); }

  // Description:
  // Adds a point to the end of the list of edge points for a certain edge.
  void AddEdgePoint(vtkIdType e, double x[3]);
  void AddEdgePoint(vtkIdType e, double x, double y, double z)
    { double p[3] = {x, y, z}; this->AddEdgePoint(e, p); }

  // Description:
  // Copy the internal edge point data from another graph into this graph.
  // Both graphs must have the same number of edges.
  void ShallowCopyEdgePoints(vtkGraph* g);
  void DeepCopyEdgePoints(vtkGraph* g);

  // Description:
  // Returns the internal representation of the graph. If modifying is
  // true, then the returned vtkGraphInternals object will be unique to
  // this vtkGraph object.
  vtkGraphInternals *GetGraphInternals(bool modifying);

  // Description:
  // Fills a list of edge indices with the edges contained in the induced
  // subgraph formed by the vertices in the vertex list.
  void GetInducedEdges(vtkIdTypeArray* verts, vtkIdTypeArray* edges);

  // Description:
  // Returns the attributes of the data object as a vtkFieldData.
  // This returns non-null values in all the same cases as GetAttributes,
  // in addition to the case of FIELD, which will return the field data
  // for any vtkDataObject subclass.
  virtual vtkFieldData* GetAttributesAsFieldData(int type);

  // Description:
  // Get the number of elements for a specific attribute type (VERTEX, EDGE, etc.).
  virtual vtkIdType GetNumberOfElements(int type);

  // Description:
  // Dump the contents of the graph to standard output.
  void Dump();

  // Description:
  // Returns the Id of the edge between vertex a and vertex b.
  // This is independent of directionality of the edge, that is,
  // if edge A->B exists or if edge B->A exists, this function will
  // return its Id. If multiple edges exist between a and b, here is no guarantee
  // about which one will be returned.
  // Returns -1 if no edge exists between a and b.
  vtkIdType GetEdgeId(vtkIdType a, vtkIdType b);

  // Description:
  // Convert the graph to a directed graph.
  bool ToDirectedGraph(vtkDirectedGraph* g);

  // Description:
  // Convert the graph to an undirected graph.
  bool ToUndirectedGraph(vtkUndirectedGraph* g);

protected:
  //BTX
  vtkGraph();
  ~vtkGraph();

  // Description:
  // Protected method for adding vertices, optionally with properties,
  // used by mutable subclasses. If vertex is non-null, it will be set
  // to the newly-added (or found) vertex. Note that if propertyArr is
  // non-null and the vertex data contains pedigree IDs, a vertex will
  // only be added if there is no vertex with that pedigree ID.
  void AddVertexInternal(vtkVariantArray *propertyArr = 0,
                         vtkIdType *vertex = 0);

  // Description:
  // Adds a vertex with the given pedigree ID to the graph. If a vertex with
  // this pedigree ID already exists, no new vertex is added, but the vertex
  // argument is set to the ID of the existing vertex.  Otherwise, a
  // new vertex is added and its ID is provided.
  void AddVertexInternal(const vtkVariant& pedigree, vtkIdType *vertex);

  // Description:
  // Protected method for adding edges of a certain directedness used
  // by mutable subclasses. If propertyArr is non-null, it specifies
  // the properties to be attached to the newly-created edge. If
  // non-null, edge will receive the newly-added edge.
  void AddEdgeInternal(vtkIdType u, vtkIdType v, bool directed,
                       vtkVariantArray *propertyArr, vtkEdgeType *edge);
  void AddEdgeInternal(const vtkVariant& uPedigree, vtkIdType v, bool directed,
                       vtkVariantArray *propertyArr, vtkEdgeType *edge);
  void AddEdgeInternal(vtkIdType u, const vtkVariant& vPedigree, bool directed,
                       vtkVariantArray *propertyArr, vtkEdgeType *edge);
  void AddEdgeInternal(const vtkVariant& uPedigree, const vtkVariant& vPedigree,
                       bool directed, vtkVariantArray *propertyArr,
                       vtkEdgeType *edge);

  // Description:
  // Removes a vertex from the graph, along with any adjacent edges.
  // This invalidates the id of the last vertex, since it is reassigned to v.
  void RemoveVertexInternal(vtkIdType v, bool directed);

  // Description:
  // Removes an edge from the graph.
  // This invalidates the id of the last edge, since it is reassigned to e.
  void RemoveEdgeInternal(vtkIdType e, bool directed);

  // Description:
  // Removes a collection of vertices from the graph, along with any adjacent edges.
  void RemoveVerticesInternal(vtkIdTypeArray* arr, bool directed);

  // Description:
  // Removes a collection of edges from the graph.
  void RemoveEdgesInternal(vtkIdTypeArray* arr, bool directed);
  //ETX

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
  // The distributed graph helper. Only non-NULL for distributed graphs.
  vtkDistributedGraphHelper *DistributedHelper;

  // Description:
  // Private method for setting internals.
  void SetInternals(vtkGraphInternals* internals);

  // Description:
  // The structure for holding the edge points.
  vtkGraphEdgePoints *EdgePoints;

  // Description:
  // Private method for setting edge points.
  void SetEdgePoints(vtkGraphEdgePoints* edgePoints);

  // Description:
  // If this instance does not own its internals, it makes a copy of the
  // internals.  This is called before any write operation.
  void ForceOwnership();

  // Description:
  // Fast access functions for iterators.
  virtual void GetOutEdges(vtkIdType v, const vtkOutEdgeType *& edges, vtkIdType & nedges);
  virtual void GetInEdges(vtkIdType v, const vtkInEdgeType *& edges, vtkIdType & nedges);

  // Description:
  // Builds a mapping from edge id to source/target vertex id.
  void BuildEdgeList();

  //BTX
  // Description:
  // Friend iterator classes.
  friend class vtkAdjacentVertexIterator;
  friend class vtkEdgeListIterator;
  friend class vtkInEdgeIterator;
  friend class vtkOutEdgeIterator;
  friend class boost::vtk_edge_iterator;
  friend class boost::vtk_in_edge_pointer_iterator;
  friend class boost::vtk_out_edge_pointer_iterator;
  //ETX

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

  // Description:
  // The optional mapping from edge id to source/target ids.
  vtkGetObjectMacro(EdgeList, vtkIdTypeArray);
  virtual void SetEdgeList(vtkIdTypeArray* list);
  vtkIdTypeArray *EdgeList;
  //ETX
private:
  vtkGraph(const vtkGraph&);  // Not implemented.
  void operator=(const vtkGraph&);  // Not implemented.
};

//BTX
bool VTKCOMMONDATAMODEL_EXPORT operator==(vtkEdgeBase e1, vtkEdgeBase e2);
bool VTKCOMMONDATAMODEL_EXPORT operator!=(vtkEdgeBase e1, vtkEdgeBase e2);
VTKCOMMONDATAMODEL_EXPORT ostream& operator<<(ostream& out, vtkEdgeBase e);
//ETX

#endif
