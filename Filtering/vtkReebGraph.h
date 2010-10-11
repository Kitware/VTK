/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkReebGraph.h,v $

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

// .NAME vtkReebGraph - Reeb graph computation for PL scalar fields.
//
// .SECTION Description
// vtkReebGraph is a class that computes a Reeb graph given a PL scalar
// field (vtkDataArray) defined on a simplicial mesh.
// A Reeb graph is a concise representation of the connectivity evolution of
// the level sets of a scalar function.
//
// It is particularly useful in visualization (optimal seed set computation,
// fast flexible isosurface extraction, automated transfer function design,
// feature-driven visualization, etc.) and computer graphics (shape
// deformation, shape matching, shape compression, etc.).
//
// Reference:
// "Sur les points singuliers d'une forme de Pfaff completement integrable ou
// d'une fonction numerique",
// G. Reeb,
// Comptes-rendus de l'Academie des Sciences, 222:847-849, 1946.
//
// vtkReebGraph implements one of the latest and most robust Reeb graph
// computation algorithms.
//
// Reference:
// "Robust on-line computation of Reeb graphs: simplicity and speed",
// V. Pascucci, G. Scorzelli, P.-T. Bremer, and A. Mascarenhas,
// ACM Transactions on Graphics, Proc. of SIGGRAPH 2007.
//
// vtkReebGraph provides methods for computing multi-resolution topological
// hierarchies through topological simplification.
// Topoligical simplification can be either driven by persistence homology
// concepts (default behavior) or by application specific metrics (see
// vtkReebGraphSimplificationMetric).
// In the latter case, designing customized simplification metric evaluation
// algorithms enables the user to control the definition of what should be
// considered as noise or signal in the topological filtering process.
//
// References:
// "Topological persistence and simplification",
// H. Edelsbrunner, D. Letscher, and A. Zomorodian,
// Discrete Computational Geometry, 28:511-533, 2002.
//
// "Extreme elevation on a 2-manifold",
// P.K. Agarwal, H. Edelsbrunner, J. Harer, and Y. Wang,
// ACM Symposium on Computational Geometry, pp. 357-365, 2004.
//
// "Simplifying flexible isosurfaces using local geometric measures",
// H. Carr, J. Snoeyink, M van de Panne,
// IEEE Visualization, 497-504, 2004
//
// "Loop surgery for volumetric meshes: Reeb graphs reduced to contour trees",
// J. Tierny, A. Gyulassy, E. Simon, V. Pascucci,
// IEEE Trans. on Vis. and Comp. Graph. (Proc of IEEE VIS), 15:1177-1184, 2009.
//
//
//
// Reeb graphs can be computed from 2D data (vtkPolyData, with triangles only)
// or 3D data (vtkUnstructuredGrid, with tetrahedra only), sequentially (see
// the "Build" calls) or in streaming (see the "StreamTriangle" and
// "StreamTetrahedron" calls).
//
// vtkReebGraph inherits from vtkMutableDirectedGraph.
//
// Each vertex of a vtkReebGraph object represents a critical point of the
// scalar field where the connectivity of the related level set changes
// (creation, deletion, split or merge of connected components).
// A vtkIdTypeArray (called "Vertex Ids") is associated with the VertexData of
// a vtkReebGraph object, in order to retrieve if necessary the exact Ids of
// the corresponding vertices in the input mesh.
//
// The edges of a vtkReebGraph object represent the regions of the input mesh
// separated by the critical contours of the field, and where the connectivity
// of the input field does not change.
// A vtkVariantArray is associated with the EdgeDta of a vtkReebGraph object and
// each entry of this array is a vtkAbstractArray containing the Ids of the
// vertices of those regions, sorted by function value (useful for flexible
// isosurface extraction or level set signature computation, for instance).
//
// See Graphics/Testing/Cxx/TestReebGraph.cxx for examples of traversals and
// typical usages (customized simplification, skeletonization, contour spectra,
//  etc.) of a vtkReebGraph object.
//
//
// .SECTION See Also
//      vtkReebGraphSimplificationMetric
//      vtkPolyDataToReebGraphFilter
//      vtkUnstructuredGridToReebGraphFilter
//      vtkReebGraphSimplificationFilter
//      vtkReebGraphSurfaceSkeletonFilter
//      vtkReebGraphVolumeSkeletonFilter
//      vtkAreaContourSpectrumFilter
//      vtkVolumeContourSpectrumFilter
//
// .SECTION Tests
//      Graphics/Testing/Cxx/TestReebGraph.cxx

#ifndef __vtkReebGraph_h
#define __vtkReebGraph_h

#include <map> //needed for internal datastructures
#include <vector> //needed for internal datastructures

#include  "vtkMutableDirectedGraph.h"

class vtkDataArray;
class vtkDataSet;
class vtkIdList;
class vtkPolyData;
class vtkReebGraphSimplificationMetric;
class vtkUnstructuredGrid;

class VTK_FILTERING_EXPORT vtkReebGraph : public vtkMutableDirectedGraph
{

public:

  static vtkReebGraph *New();

  vtkTypeMacro(vtkReebGraph, vtkMutableDirectedGraph);
  void PrintSelf(ostream& os, vtkIndent indent);
  void PrintNodeData(ostream& os, vtkIndent indent);

  enum
  {
    ERR_INCORRECT_FIELD = -1,
    ERR_NO_SUCH_FIELD = -2,
    ERR_NOT_A_SIMPLICIAL_MESH = -3
  };

  // Description:
  // Build the Reeb graph of the field 'scalarField' defined on the surface
  // mesh 'mesh'.
  //
  // Returned values:
  //
  // vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
  // tuples as 'mesh' has vertices.
  //
  // vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
  // simplicial mesh (for example, the surface mesh contains quads instead of
  // triangles).
  //
  int Build(vtkPolyData *mesh, vtkDataArray *scalarField);

  // Description:
  // Build the Reeb graph of the field 'scalarField' defined on the volume
  // mesh 'mesh'.
  //
  // Returned values:
  //
  // vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
  // tuples as 'mesh' has vertices.
  //
  // vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
  // simplicial mesh.
  //
  int Build(vtkUnstructuredGrid *mesh, vtkDataArray *scalarField);


  // Description:
  // Build the Reeb graph of the field given by the Id 'scalarFieldId',
  // defined on the surface mesh 'mesh'.
  //
  // Returned values:
  //
  // vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
  // tuples as 'mesh' as vertices.
  //
  // vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
  // simplicial mesh (for example, the surface mesh contains quads instead of
  // triangles).
  //
  // vtkReebGraph::ERR_NO_SUCH_FIELD: the scalar field given by the Id
  // 'scalarFieldId' does not exist.
  //
  int Build(vtkPolyData *mesh, vtkIdType scalarFieldId);

  // Description:
  // Build the Reeb graph of the field given by the Id 'scalarFieldId',
  // defined on the volume mesh 'mesh'.
  //
  // Returned values:
  //
  // vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
  // tuples as 'mesh' as vertices.
  //
  // vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
  // simplicial mesh.
  //
  // vtkReebGraph::ERR_NO_SUCH_FIELD: the scalar field given by the Id
  // 'scalarFieldId' does not exist.
  //
  int Build(vtkUnstructuredGrid *mesh, vtkIdType scalarFieldId);


  // Description:
  // Build the Reeb graph of the field given by the name 'scalarFieldName',
  // defined on the surface mesh 'mesh'.
  //
  // Returned values:
  //
  // vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
  // tuples as 'mesh' as vertices.
  //
  // vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
  // simplicial mesh (for example, the surface mesh contains quads instead of
  // triangles).
  //
  // vtkReebGraph::ERR_NO_SUCH_FIELD: the scalar field given by the name
  // 'scalarFieldName' does not exist.
  //
  int Build(vtkPolyData *mesh, const char* scalarFieldName);

  // Description:
  // Build the Reeb graph of the field given by the name 'scalarFieldName',
  // defined on the volume mesh 'mesh'.
  //
  // Returned values:
  //
  // vtkReebGraph::ERR_INCORRECT_FIELD: 'scalarField' does not have as many
  // tuples as 'mesh' as vertices.
  //
  // vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH: the input mesh 'mesh' is not a
  // simplicial mesh.
  //
  // vtkReebGraph::ERR_NO_SUCH_FIELD: the scalar field given by the name
  // 'scalarFieldName' does not exist.
  //
  int Build(vtkUnstructuredGrid *mesh, const char* scalarFieldName);

  // Description:
  // Streaming Reeb graph computation.
  // Add to the streaming computation the triangle of the vtkPolyData surface
  // mesh described by
  // 	vertex0Id, scalar0
  // 	vertex1Id, scalar1
  // 	vertex2Id, scalar2
  //
  // 	where vertex<i>Id is the Id of the vertex in the vtkPolyData structure
  // 	and scalar<i> is the corresponding scalar field value.
  //
  // IMPORTANT: The stream _must_ be finalized with the "CloseStream" call.
  int StreamTriangle(	vtkIdType vertex0Id, double scalar0,
                      vtkIdType vertex1Id, double scalar1,
                      vtkIdType vertex2Id, double scalar2);

  // Description:
  // Streaming Reeb graph computation.
  // Add to the streaming computation the tetrahedra of the vtkUnstructuredGrid
  // volume mesh described by
  // 	vertex0Id, scalar0
  // 	vertex1Id, scalar1
  // 	vertex2Id, scalar2
  // 	vertex3Id, scalar3
  //
  // 	where vertex<i>Id is the Id of the vertex in the vtkUnstructuredGrid
  // 	structure and scalar<i> is the corresponding scalar field value.
  //
  // IMPORTANT: The stream _must_ be finalized with the "CloseStream" call.
  int StreamTetrahedron( vtkIdType vertex0Id, double scalar0,
                         vtkIdType vertex1Id, double scalar1,
                         vtkIdType vertex2Id, double scalar2,
                         vtkIdType vertex3Id, double scalar3);

  // Description:
  // Finalize internal data structures, in the case of streaming computations
  // (with StreamTriangle or StreamTetrahedron).
  // After this call, no more triangle or tetrahedron can be inserted via
  // StreamTriangle or StreamTetrahedron.
  // IMPORTANT: This method _must_ be called when the input stream is finished.
  // If you need to get a snapshot of the Reeb graph during the streaming
  // process (to parse or simplify it), do a DeepCopy followed by a
  // CloseStream on the copy.
  void CloseStream();

  // Descrition:
  // Implements deep copy
  void DeepCopy(vtkDataObject *src);

  // Description:
  // Simplify the Reeb graph given a threshold 'simplificationThreshold'
  // (between 0 and 1).
  //
  // This method is the core feature for Reeb graph multi-resolution hierarchy
  // construction.
  //
  // Return the number of arcs that have been removed through the simplification
  // process.
  //
  // 'simplificationThreshold' represents a "scale", under which each Reeb graph
  // feature is considered as noise. 'simplificationThreshold' is expressed as a
  // fraction of the scalar field overall span. It can vary from 0
  // (no simplification) to 1 (maximal simplification).
  //
  // 'simplificationMetric' is an object in charge of evaluating the importance
  // of a Reeb graph arc at each step of the simplification process.
  // if 'simplificationMetric' is NULL, the default strategy (persitence of the
  // scalar field) is used.
  // Customized simplification metric evaluation algorithm can be designed (see
  // vtkReebGraphSimplificationMetric), enabling the user to control the
  // definition of what should be considered as noise or signal.
  //
  // References:
  //
  // "Topological persistence and simplification",
  // H. Edelsbrunner, D. Letscher, and A. Zomorodian,
  // Discrete Computational Geometry, 28:511-533, 2002.
  //
  // "Extreme elevation on a 2-manifold",
  // P.K. Agarwal, H. Edelsbrunner, J. Harer, and Y. Wang,
  // ACM Symposium on Computational Geometry, pp. 357-365, 2004.
  //
  // "Simplifying flexible isosurfaces using local geometric measures",
  // H. Carr, J. Snoeyink, M van de Panne,
  // IEEE Visualization, 497-504, 2004
  //
  // "Loop surgery for volumetric meshes: Reeb graphs reduced to contour trees",
  // J. Tierny, A. Gyulassy, E. Simon, V. Pascucci,
  // IEEE Trans. on Vis. and Comp. Graph. (Proc of IEEE VIS), 15:1177-1184,2009.
  int Simplify(double simplificationThreshold,
    vtkReebGraphSimplificationMetric *simplificationMetric);

  // Description:
  // Use a pre-defined Reeb graph (post-processing).
  // Use with caution!
  void Set(vtkMutableDirectedGraph *g);

protected:

  vtkReebGraph();
  ~vtkReebGraph();


  // INTERNAL DATA-STRUCTURES ------------------------------------------------


  // Streaming support
  int										VertexMapSize, VertexMapAllocatedSize,
                        TriangleVertexMapSize, TriangleVertexMapAllocatedSize;
  std::map<int, int> 		VertexStream;

  typedef unsigned long long vtkReebLabelTag;

  bool                  historyOn;

  typedef struct        _vtkReebCancellation{
    std::vector<std::pair<int, int> > removedArcs;
    std::vector<std::pair<int, int> > insertedArcs;
  }vtkReebCancellation;
  std::vector<vtkReebCancellation> cancellationHistory;

  // Node structure
  typedef struct
  {
    vtkIdType  VertexId;
    double  Value;
    vtkIdType  ArcDownId;
    vtkIdType  ArcUpId;
    bool  IsFinalized;
    bool IsCritical;
  } vtkReebNode;

  // Arc structure
  typedef struct
  {
    vtkIdType  NodeId0, ArcUpId0, ArcDwId0;
    vtkIdType  NodeId1, ArcUpId1, ArcDwId1;
    vtkIdType  LabelId0, LabelId1;
  } vtkReebArc;

  // Label structure
  typedef struct
  {
    vtkIdType  ArcId;
    vtkIdType  HPrev, HNext; // "horizontal" (for a single arc)
    vtkReebLabelTag label;
    vtkIdType  VPrev, VNext; // "vertical" (for a sequence of arcs)
  } vtkReebLabel;

  struct vtkReebPath
  {
//    double  MinimumScalarValue, MaximumScalarValue;
    double  SimplificationValue;
    int  ArcNumber;
    vtkIdType*  ArcTable;
    int  NodeNumber;
    vtkIdType* NodeTable;

    inline bool operator<( struct vtkReebPath const &E ) const
    {
      return !(
          (SimplificationValue < E.SimplificationValue) ||
          (SimplificationValue == E.SimplificationValue
            && ArcNumber < E.ArcNumber) ||
          (SimplificationValue == E.SimplificationValue
            && ArcNumber == E.ArcNumber
            && NodeTable[NodeNumber - 1] < E.NodeTable[E.NodeNumber - 1]));
/*      return !((
          (MaximumScalarValue - MinimumScalarValue)
            < (E.MaximumScalarValue - E.MinimumScalarValue)) ||
             ((MaximumScalarValue - MinimumScalarValue)
               == (E.MaximumScalarValue-E.MinimumScalarValue)
                 && ArcNumber < E.ArcNumber) ||
             ((MaximumScalarValue - MinimumScalarValue)
               == (E.MaximumScalarValue - E.MinimumScalarValue)
                 && ArcNumber == E.ArcNumber
                   && NodeTable[NodeNumber - 1]<E.NodeTable[E.NodeNumber - 1])
           );*/

    }
  };

  vtkReebGraph::vtkReebPath FindPath(vtkIdType arcId,
    double simplificationThreshold, vtkReebGraphSimplificationMetric *metric);

  struct
  {
    int Size, Number, FreeZone;
    vtkReebArc* Buffer;
  } MainArcTable;


  struct
  {
    int Size, Number, FreeZone;
    vtkReebNode* Buffer;
  } MainNodeTable;


  struct
  {
    int Size, Number, FreeZone;
    vtkReebLabel* Buffer;
  } MainLabelTable;

  vtkIdType  *VertexMap;
  int *TriangleVertexMap;

  double MinimumScalarValue, MaximumScalarValue;

  // Arcs and nodes
  int ArcNumber, NodeNumber;

  // Loops
  int LoopNumber, RemovedLoopNumber;
  vtkIdType *ArcLoopTable;

  // CC
  int ConnectedComponentNumber;

  std::map<int, double> ScalarField;

  vtkIdType currentNodeId, currentArcId;

  vtkDataSet            *inputMesh;
  vtkDataArray          *inputScalarField;

  // INTERNAL METHODS --------------------------------------------------------

  // Description:
  //  Add a monotonic path between nodes.
  //
  //  INTERNAL USE ONLY!
  vtkIdType AddPath(int nodeNumber, vtkIdType* nodeOffset,
                    vtkReebLabelTag label);

  // Description:
  //   Add a vertex from the mesh to the Reeb graph.
  //
  //   INTERNAL USE ONLY!
  vtkIdType AddMeshVertex(vtkIdType vertexId, double scalar);

  // Description:
  //   Add a triangle from the mesh to the Reeb grpah.
  //
  //   INTERNAL USE ONLY!
  int AddMeshTriangle(vtkIdType vertex0Id, double f0,
    vtkIdType vertex1Id, double f1, vtkIdType vertex2Id, double f2);

  // Description:
  //   Add a tetrahedron from the mesh to the Reeb grpah.
  //
  //   INTERNAL USE ONLY!
  int AddMeshTetrahedron(vtkIdType vertex0Id, double f0,
    vtkIdType vertex1Id, double f1, vtkIdType vertex2Id, double f2,
    vtkIdType vertex3Id, double f3);

  // Description:
  // "Zip" the corresponding paths when the interior of a simplex is added to
  // the Reeb graph.
  //
  // INTERNAL USE ONLY!
  void Collapse(vtkIdType startingNode, vtkIdType endingNode,
                vtkReebLabelTag startingLabel, vtkReebLabelTag endingLabel);

  // Description:
  // Finalize a vertex.
  //
  // INTERNAL USE ONLY!
  void EndVertex(const vtkIdType N);

  // Description:
  // Remove an arc during filtering by persistence.
  //
  // INTERNAL USE ONLY!
  void FastArcSimplify(vtkIdType arcId, int arcNumber, vtkIdType* arcTable);

  // Description:
  // Triggers customized code for simplification metric evaluation.
  //
  // INTERNAL USE ONLY!
  double ComputeCustomMetric(
    vtkReebGraphSimplificationMetric *simplificationMetric,
    vtkReebArc *a);

  // Description:
  // Remove arcs below the provided persistence.
  //
  // INTERNAL USE ONLY!
  int SimplifyBranches(double simplificationThreshold,
    vtkReebGraphSimplificationMetric *simplificationMetric);

  // Description:
  // Remove the loops below the provided persistence.
  //
  // INTERNAL USE ONLY!
  int SimplifyLoops(double simplificationThreshold,
    vtkReebGraphSimplificationMetric *simplificationMetric);

  // Description:
  // Update the vtkMutableDirectedGraph internal structure after filtering, with
  // deg-2 nodes maintaining.
  //
  // INTERNAL USE ONLY!
  int CommitSimplification();

  // Description:
  // Retrieve downwards labels.
  //
  // INTERNAL USE ONLY!
  vtkIdType FindDwLabel(vtkIdType nodeId, vtkReebLabelTag label);

  // Description
  // Find greater arc (persistence-based simplification).
  //
  // INTERNAL USE ONLY!
  vtkIdType FindGreater(vtkIdType nodeId, vtkIdType startingNodeId,
                        vtkReebLabelTag label);


  // Description:
  // Find corresponding joining saddle node (persistence-based simplification).
  //
  // INTERNAL USE ONLY!
  vtkIdType FindJoinNode(vtkIdType arcId,
    vtkReebLabelTag label, bool onePathOnly=false);

  // Description:
  // Find smaller arc (persistence-based simplification).
  //
  // INTERNAL USE ONLY!
  vtkIdType FindLess(vtkIdType nodeId, vtkIdType startingNodeId,
                     vtkReebLabelTag label);

  // Description:
  // Compute the loops in the Reeb graph.
  //
  // INTERNAL USE ONLY!
  void FindLoops();

  // Description:
  // Find corresponding splitting saddle node (persistence-based
  // simplification).
  //
  // INTERNAL USE ONLY!
  vtkIdType FindSplitNode(vtkIdType arcId, vtkReebLabelTag label,
    bool onePathOnly=false);

  // Description:
  // Retrieve upwards labels.
  //
  // INTERNAL USE ONLY!
  vtkIdType FindUpLabel(vtkIdType nodeId, vtkReebLabelTag label);


  // Description:
  // Flush labels.
  //
  // INTERNAL USE ONLY!
  void FlushLabels();

  // Description:
  // Resize the arc table.
  //
  // INTERNAL USE ONLY!
  void ResizeMainArcTable(int newSize);

  // Description:
  // Resize the label table.
  //
  // INTERNAL USE ONLY!
  void ResizeMainLabelTable(int newSize);

  // Description:
  // Resize the node table.
  //
  // INTERNAL USE ONLY!
  void ResizeMainNodeTable(int newSize);

  // Description:
  // Set a label.
  //
  // INTERNAL USE ONLY!
  void SetLabel(vtkIdType A, vtkReebLabelTag Label);

  // Description:
  // Simplify labels.
  //
  // INTERNAL USE ONLY!
  void SimplifyLabels(const vtkIdType nodeId, vtkReebLabelTag onlyLabel=0,
                      bool goDown=true, bool goUp=true);

  // ACCESSORS

  // Description:
  // Returns the Id of the lower node of the arc specified by 'arcId'.
  vtkIdType GetArcDownNodeId(vtkIdType arcId);

  // Description:
  // Return the Id of the upper node of the arc specified by 'arcId'.
  vtkIdType GetArcUpNodeId(vtkIdType arcId);

  // Description:
  // Iterates forwards through the arcs of the Reeb graph.
  //
  // The first time this method is called, the first arc's Id will be returned.
  // When the last arc is reached, this method will keep on returning its Id at
  // each call. See 'GetPreviousArcId' to go back in the list.
  vtkIdType GetNextArcId();

  // Description:
  // Iterates forwards through the nodes of the Reeb graph.
  //
  // The first time this method is called, the first node's Id will be returned.
  // When the last node is reached, this method will keep on returning its Id at
  // each call. See 'GetPreviousNodeId' to go back in the list.
  vtkIdType GetNextNodeId();

  // Description:
  // Copy into 'arcIdList' the list of the down arcs' Ids, given a node
  // specified by 'nodeId'.
  void GetNodeDownArcIds(vtkIdType nodeId, vtkIdList *arcIdList);

  // Description:
  // Returns the scalar field value of the node specified by 'nodeId'.
  double GetNodeScalarValue(vtkIdType nodeId);

  // Description:
  // Copy into 'arcIdList' the list of the up arcs' Ids, given a node specified
  // by 'nodeId'.
  void GetNodeUpArcIds(vtkIdType nodeId, vtkIdList *arcIdList);

  // Description:
  // Returns the corresponding vertex Id (in the simplicial mesh, vtkPolyData),
  // given a node specified by 'nodeId'.
  vtkIdType GetNodeVertexId(vtkIdType nodeId);

  // Description:
  // Returns the number of arcs in the Reeb graph.
  int GetNumberOfArcs();

  // Description:
  // Returns the number of connected components of the Reeb graph.
  int GetNumberOfConnectedComponents();

  // Description:
  // Returns the number of nodes in the Reeb graph.
  int GetNumberOfNodes();

  // Description:
  // Returns the number of loops (cycles) in the Reeb graph.
  //
  // Notice that for closed PL 2-manifolds, this number equals the genus of the
  // manifold.
  //
  // Reference:
  // "Loops in Reeb graphs of 2-manifolds",
  // K. Cole-McLaughlin, H. Edelsbrunner, J. Harer, V. Natarajan, and V.
  // Pascucci,
  // ACM Symposium on Computational Geometry, pp. 344-350, 2003.
  int GetNumberOfLoops();

  // Description:
  // Iterates backwards through the arcs of the Reeb graph.
  //
  // When the first arc is reached, this method will keep on returning its Id at
  // each call. See 'GetNextArcId' to go forwards in the list.
  vtkIdType GetPreviousArcId();

  // Description:
  // Iterates backwards through the nodes of the Reeb graph.
  //
  // When the first node is reached, this method will keep on returning its Id
  // at each call. See 'GetNextNodeId' to go forwards in the list.
  vtkIdType GetPreviousNodeId();

private:
  vtkReebGraph(const vtkReebGraph&); // Not implemented.
  void operator=(const vtkReebGraph&); // Not implemented.

  vtkIdType AddArc(vtkIdType nodeId0, vtkIdType nodeId1);

};

//----------------------------------------------------------------------------
inline static bool vtkReebGraphVertexSoS(const std::pair<int, double> v0,
  const std::pair<int, double> v1)
{
  return ((v0.second < v1.second)
    || ((v0.second == v1.second)&&(v0.first < v1.first)));
}
#endif
