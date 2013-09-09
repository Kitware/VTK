/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkReebGraph.cxx,v $

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

#include "vtkReebGraph.h"

#include "vtkCell.h"
#include "vtkEdgeListIterator.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkReebGraphSimplificationMetric.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVariantArray.h"

#include <vector>
#include <map>
#include <algorithm>
#include <queue>

//----------------------------------------------------------------------------
// Contain all of the internal data structures, and macros, in the
// implementation.
namespace
{
//----------------------------------------------------------------------------
inline static bool vtkReebGraphVertexSoS(const std::pair<int, double> v0,
  const std::pair<int, double> v1)
{
  return ((v0.second < v1.second)
    || ((v0.second == v1.second)&&(v0.first < v1.first)));
}
}

// INTERNAL MACROS ---------------------------------------------------------
#define vtkReebGraphSwapVars(type, var1, var2)  \
{\
  type tmp;\
  tmp=(var1);\
  (var1)=(var2);\
  (var2)=tmp;\
}

#define vtkReebGraphInitialStreamSize 1000

#define vtkReebGraphIsSmaller(myReebGraph, nodeId0, nodeId1, node0, node1) \
((node0->Value < node1->Value) || (node0->Value == node1->Value && nodeId0 < nodeId1))

#define vtkReebGraphGetArcPersistence(rg,a)  \
((this->GetNode(a->NodeId1)->Value - this->GetNode(a->NodeId0)->Value) \
/(this->MaximumScalarValue - this->MinimumScalarValue))

#define vtkReebGraphIsHigherThan(rg,N0,N1,n0,n1) \
((n0->Value >n1->Value) || (n0->Value==n1->Value && n0->VertexId>n1->VertexId))

// Note: usually this macro is called after the node has been finalized,
// otherwise the behaviour is undefined.
#define vtkReebGraphIsRegular(rg,n) \
((!(n)->IsCritical) && \
((n)->ArcDownId && !this->GetArc((n)->ArcDownId)->ArcDwId1 && \
(n)->ArcUpId && !this->GetArc((n)->ArcUpId)->ArcDwId0))

#define vtkReebGraphAddUpArc(rg,N,A) {\
vtkReebNode* n = this->GetNode(N);\
  vtkReebArc* a = this->GetArc(A);           \
a->ArcUpId0=0;a->ArcDwId0=n->ArcUpId;\
if (n->ArcUpId) this->GetArc(n->ArcUpId)->ArcUpId0=(A);\
n->ArcUpId=(A);\
}

#define vtkReebGraphAddDownArc(rg,N,A) {\
vtkReebNode* n = this->GetNode(N);\
vtkReebArc* a = this->GetArc(A);\
a->ArcUpId1=0;\
a->ArcDwId1=n->ArcDownId;\
if (n->ArcDownId) this->GetArc(n->ArcDownId)->ArcUpId1=(A);\
n->ArcDownId=(A);\
}

#define vtkReebGraphRemoveUpArc(rg,N,A) {\
vtkReebNode* n = this->GetNode(N);\
vtkReebArc* a = this->GetArc(A);\
if (a->ArcUpId0) this->GetArc(a->ArcUpId0)->ArcDwId0=a->ArcDwId0; else n->ArcUpId=a->ArcDwId0;\
if (a->ArcDwId0) this->GetArc(a->ArcDwId0)->ArcUpId0=a->ArcUpId0;}

#define vtkReebGraphRemoveDownArc(rg,N,A) {\
vtkReebNode* n = this->GetNode(N);\
vtkReebArc* a = this->GetArc(A);\
if (a->ArcUpId1) this->GetArc(a->ArcUpId1)->ArcDwId1=a->ArcDwId1; else n->ArcDownId=a->ArcDwId1;\
if (a->ArcDwId1) this->GetArc(a->ArcDwId1)->ArcUpId1=a->ArcUpId1;\
}

#ifndef vtkReebGraphMax
#define vtkReebGraphMax(a,b) (((a)>=(b))?(a):(b))
#endif

#define vtkReebGraphStackPush(N) \
{\
if (nstack==mstack)\
{\
  mstack=vtkReebGraphMax(128,mstack*2);\
  stack=(int*)realloc(stack,sizeof(int)*mstack);\
} \
stack[nstack++]=(N);\
}

#define vtkReebGraphStackSize()  (nstack)

#define vtkReebGraphStackTop()   (stack[nstack-1])

#define vtkReebGraphStackPop()   (--nstack)

//----------------------------------------------------------------------------
// PIMPLed classes...
class vtkReebGraph::Implementation
{
public:
  Implementation()
    {
    this->historyOn = false;

    this->MainNodeTable.Buffer = (vtkReebNode*)malloc(sizeof(vtkReebNode)*2);
    this->MainArcTable.Buffer = (vtkReebArc*)malloc(sizeof(vtkReebArc)*2);
    this->MainLabelTable.Buffer = (vtkReebLabel*)malloc(sizeof(vtkReebLabel)*2);

    this->MainNodeTable.Size=2;
    this->MainNodeTable.Number=1; //the item "0" is blocked
    this->MainArcTable.Size=2;
    this->MainArcTable.Number=1;
    this->MainLabelTable.Size=2;
    this->MainLabelTable.Number=1;

    this->MainNodeTable.FreeZone=1;
    // Clear node
    this->GetNode(1)->ArcUpId = ((int)-2);
    // Intialize DownArc
    this->GetNode(1)->ArcDownId = 0;
    this->MainArcTable.FreeZone=1;
    // Clear Arc label 1
    this->GetArc(1)->LabelId1 = ((int)-2);
    // Initialize Arc label 0
    this->GetArc(1)->LabelId0 = 0;
    this->MainLabelTable.FreeZone=1;
    // Clear label
    this->GetLabel(1)->HNext = ((int)-2);
    // Initialize Arc id
    this->GetLabel(1)->ArcId = 0;

    this->MinimumScalarValue = 0;
    this->MaximumScalarValue = 0;

    this->ArcNumber = 0;
    this->NodeNumber = 0;
    this->LoopNumber = 0;
    this->RemovedLoopNumber = 0;
    this->ArcLoopTable = 0;

    this->currentNodeId = 0;
    this->currentArcId = 0;

    // streaming support
    this->VertexMapSize = 0;
    this->VertexMapAllocatedSize = 0;
    this->TriangleVertexMapSize = 0;
    this->TriangleVertexMapAllocatedSize = 0;
    }
  ~Implementation()
    {
    free(this->MainNodeTable.Buffer);
    this->MainNodeTable.Buffer = NULL;

    free(this->MainArcTable.Buffer);
    this->MainArcTable.Buffer = NULL;

    free(this->MainLabelTable.Buffer);
    this->MainLabelTable.Buffer = NULL;

    this->MainNodeTable.Size = this->MainNodeTable.Number = 0;
    this->MainArcTable.Size = this->MainArcTable.Number = 0;
    this->MainLabelTable.Size = this->MainLabelTable.Number = 0;

    this->MainNodeTable.FreeZone = 0;
    this->MainArcTable.FreeZone = 0;
    this->MainLabelTable.FreeZone = 0;

    if(this->ArcLoopTable) free(this->ArcLoopTable);

    if(this->VertexMapAllocatedSize) free(this->VertexMap);

    if(this->TriangleVertexMapAllocatedSize) free(this->TriangleVertexMap);
    }

  typedef unsigned long long vtkReebLabelTag;

  typedef struct _vtkReebCancellation{
    std::vector<std::pair<int, int> > removedArcs;
    std::vector<std::pair<int, int> > insertedArcs;
  }vtkReebCancellation;

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

  vtkReebPath FindPath(vtkIdType arcId,
    double simplificationThreshold, vtkReebGraphSimplificationMetric *metric);

  // INTERNAL METHODS --------------------------------------------------------

  vtkIdType AddArc(vtkIdType nodeId0, vtkIdType nodeId1);

  // Description:
  // Get the node specified from the graph.
  vtkReebNode* GetNode(vtkIdType nodeId);

  // Description:
  // Get the arc specified from the graph.
  vtkReebArc* GetArc(vtkIdType arcId);

  // Description:
  // Get the Label specified from the graph.
  vtkReebLabel* GetLabel(vtkIdType labelId);

  // Description:
  // Collapse (consolidate) two nodes in the graph.
  void CollapseVertex(vtkIdType N, vtkReebNode* n);

  // Description:
  // Triggers customized code for simplification metric evaluation.
  double ComputeCustomMetric(
    vtkReebGraphSimplificationMetric *simplificationMetric,
    vtkReebArc *a);

  // Description:
  //  Add a monotonic path between nodes.
  vtkIdType AddPath(int nodeNumber, vtkIdType* nodeOffset,
                    vtkReebLabelTag label);

  // Description:
  //   Add a vertex from the mesh to the Reeb graph.
  vtkIdType AddMeshVertex(vtkIdType vertexId, double scalar);

  // Description:
  //   Add a triangle from the mesh to the Reeb grpah.
  int AddMeshTriangle(vtkIdType vertex0Id, double f0,
    vtkIdType vertex1Id, double f1, vtkIdType vertex2Id, double f2);

  // Description:
  //   Add a tetrahedron from the mesh to the Reeb grpah.
  int AddMeshTetrahedron(vtkIdType vertex0Id, double f0,
    vtkIdType vertex1Id, double f1, vtkIdType vertex2Id, double f2,
    vtkIdType vertex3Id, double f3);

  // Description:
  // "Zip" the corresponding paths when the interior of a simplex is added to
  // the Reeb graph.
  void Collapse(vtkIdType startingNode, vtkIdType endingNode,
                vtkReebLabelTag startingLabel, vtkReebLabelTag endingLabel);

  // Description:
  // Finalize a vertex.
  void EndVertex(const vtkIdType N);

  // Description:
  // Remove an arc during filtering by persistence.
  void FastArcSimplify(vtkIdType arcId, int arcNumber, vtkIdType* arcTable);

  // Description:
  // Remove arcs below the provided persistence.
  int SimplifyBranches(double simplificationThreshold,
    vtkReebGraphSimplificationMetric *simplificationMetric);

  // Description:
  // Remove the loops below the provided persistence.
  int SimplifyLoops(double simplificationThreshold,
    vtkReebGraphSimplificationMetric *simplificationMetric);

  // Description:
  // Update the vtkMutableDirectedGraph internal structure after filtering, with
  // deg-2 nodes maintaining.
  int CommitSimplification();

  // Description:
  // Retrieve downwards labels.
  vtkIdType FindDwLabel(vtkIdType nodeId, vtkReebLabelTag label);

  // Description
  // Find greater arc (persistence-based simplification).
  vtkIdType FindGreater(vtkIdType nodeId, vtkIdType startingNodeId,
                        vtkReebLabelTag label);

  // Description:
  // Find corresponding joining saddle node (persistence-based simplification).
  vtkIdType FindJoinNode(vtkIdType arcId,
    vtkReebLabelTag label, bool onePathOnly=false);

  // Description:
  // Find smaller arc (persistence-based simplification).
  vtkIdType FindLess(vtkIdType nodeId, vtkIdType startingNodeId,
                     vtkReebLabelTag label);

  // Description:
  // Compute the loops in the Reeb graph.
  void FindLoops();

  // Description:
  // Find corresponding splitting saddle node (persistence-based
  // simplification).
  vtkIdType FindSplitNode(vtkIdType arcId, vtkReebLabelTag label,
    bool onePathOnly=false);

  // Description:
  // Retrieve upwards labels.
  vtkIdType FindUpLabel(vtkIdType nodeId, vtkReebLabelTag label);

  // Description:
  // Flush labels.
  void FlushLabels();

  // Description:
  // Resize the arc table.
  void ResizeMainArcTable(int newSize);

  // Description:
  // Resize the label table.
  void ResizeMainLabelTable(int newSize);

  // Description:
  // Resize the node table.
  void ResizeMainNodeTable(int newSize);

  // Description:
  // Set a label.
  void SetLabel(vtkIdType A, vtkReebLabelTag Label);

  // Description:
  // Simplify labels.
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

  // Description:
  // Implementations of the stream classes to operate on the streams...
  int StreamTetrahedron( vtkIdType vertex0Id, double scalar0,
                         vtkIdType vertex1Id, double scalar1,
                         vtkIdType vertex2Id, double scalar2,
                         vtkIdType vertex3Id, double scalar3);
  int StreamTriangle(	vtkIdType vertex0Id, double scalar0,
                      vtkIdType vertex1Id, double scalar1,
                      vtkIdType vertex2Id, double scalar2);

  void DeepCopy(Implementation *src);

  // Description:
  // Data storage.
  std::map<int, int> VertexStream;
  std::vector<vtkReebCancellation> cancellationHistory;
  // Streaming support
  int VertexMapSize, VertexMapAllocatedSize, TriangleVertexMapSize, TriangleVertexMapAllocatedSize;
  bool historyOn;

  vtkIdType *VertexMap;
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

  vtkDataSet *inputMesh;
  vtkDataArray *inputScalarField;

  vtkReebGraph *Parent;
};

//----------------------------------------------------------------------------
vtkReebGraph::Implementation::vtkReebNode* vtkReebGraph::Implementation::GetNode(vtkIdType nodeId)
{
  return (this->MainNodeTable.Buffer + nodeId);
}

//----------------------------------------------------------------------------
vtkReebGraph::Implementation::vtkReebArc* vtkReebGraph::Implementation::GetArc(vtkIdType arcId)
{
  return (this->MainArcTable.Buffer + arcId);
}

//----------------------------------------------------------------------------
vtkReebGraph::Implementation::vtkReebLabel* vtkReebGraph::Implementation::GetLabel(vtkIdType labelId)
{
  return (this->MainLabelTable.Buffer + labelId);
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::CollapseVertex(vtkIdType N, vtkReebNode* n)
{
  int Lb, Lnext, La;
  vtkReebLabel* lb;

  int _A0 = n->ArcDownId;
  int _A1 = n->ArcUpId;

  vtkReebArc* _a0 = this->GetArc(_A0);
  vtkReebArc* _a1 = this->GetArc(_A1);

  _a0->NodeId1  = _a1->NodeId1;
  _a0->ArcUpId1 = _a1->ArcUpId1;

  if (_a1->ArcUpId1)
    this->GetArc(_a1->ArcUpId1)->ArcDwId1 = _A0;

  _a0->ArcDwId1 = _a1->ArcDwId1;

  if(_a1->ArcDwId1)
    this->GetArc(_a1->ArcDwId1)->ArcUpId1 = _A0;

  if (this->GetNode(_a1->NodeId1)->ArcDownId == _A1)
    this->GetNode(_a1->NodeId1)->ArcDownId = _A0;

  for( Lb = _a1->LabelId0; Lb; Lb=Lnext )
    {
    lb = this->GetLabel(Lb);
    Lnext = lb->HNext;

    if (lb->VPrev)
      {
      La=lb->VPrev;
      this->GetLabel(La)->VNext = lb->VNext;
      }

    if (lb->VNext)
      this->GetLabel(lb->VNext)->VPrev=lb->VPrev;

    //delete the label...
    this->GetLabel(Lb)->HNext = ((int)-2);
    this->GetLabel(Lb)->ArcId = this->MainLabelTable.FreeZone;
    this->MainLabelTable.FreeZone=(Lb);
    --(this->MainLabelTable.Number);
    }

  //delete the arc from the graph...
  this->GetArc(_A1)->LabelId1 = ((int)-2);
  this->GetArc(_A1)->LabelId0 = this->MainArcTable.FreeZone;
  this->MainArcTable.FreeZone=(_A1);
  --(this->MainArcTable.Number);

  //delete the node from the graph...
  this->GetNode(N)->ArcUpId = ((int)-2);
  this->GetNode(N)->ArcDownId = this->MainNodeTable.FreeZone;
  this->MainNodeTable.FreeZone=(N);
  --(this->MainNodeTable.Number);
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::DeepCopy(Implementation *srcG)
{
  MinimumScalarValue = srcG->MinimumScalarValue;
  MaximumScalarValue = srcG->MaximumScalarValue;

  inputMesh = srcG->inputMesh;
  inputScalarField = srcG->inputScalarField;

  ArcNumber = srcG->ArcNumber;
  NodeNumber = srcG->NodeNumber;
  LoopNumber = srcG->LoopNumber;

  ScalarField = srcG->ScalarField;

  VertexStream = srcG->VertexStream;

  if (srcG->MainArcTable.Buffer)
  {
    MainArcTable.Size = srcG->MainArcTable.Size;
    MainArcTable.Number = srcG->MainArcTable.Number;
    this->MainArcTable.Buffer = (vtkReebArc*)malloc(
      sizeof(vtkReebArc)*srcG->MainArcTable.Size);

    memcpy(this->MainArcTable.Buffer,srcG->MainArcTable.Buffer,
      sizeof(vtkReebArc)*srcG->MainArcTable.Size);
  }

  if (srcG->MainNodeTable.Buffer)
  {
    MainNodeTable.Size = srcG->MainNodeTable.Size;
    MainNodeTable.Number = srcG->MainNodeTable.Number;
    this->MainNodeTable.Buffer = (vtkReebNode*)malloc(
      sizeof(vtkReebNode)*srcG->MainNodeTable.Size);

    memcpy(this->MainNodeTable.Buffer,srcG->MainNodeTable.Buffer,
      sizeof(vtkReebNode)*srcG->MainNodeTable.Size);
  }

  if (srcG->MainLabelTable.Buffer)
  {
    MainLabelTable.Size = srcG->MainLabelTable.Size;
    MainLabelTable.Number = srcG->MainLabelTable.Number;

    this->MainLabelTable.Buffer = (vtkReebLabel*)malloc(
      sizeof(vtkReebLabel)*srcG->MainLabelTable.Size);
    memcpy(this->MainLabelTable.Buffer,srcG->MainLabelTable.Buffer,
      sizeof(vtkReebLabel)*srcG->MainLabelTable.Size);
  }

  if (srcG->ArcLoopTable)
  {
    this->ArcLoopTable=(vtkIdType *)malloc(sizeof(vtkIdType)*srcG->LoopNumber);
    memcpy(this->ArcLoopTable,
      srcG->ArcLoopTable,sizeof(vtkIdType)*srcG->LoopNumber);
  }

  if(srcG->VertexMapSize){
    this->VertexMapSize = srcG->VertexMapSize;
    this->VertexMapAllocatedSize = srcG->VertexMapAllocatedSize;
    this->VertexMap = (vtkIdType *) malloc(
      sizeof(vtkIdType)*this->VertexMapAllocatedSize);
    memcpy(this->VertexMap, srcG->VertexMap,
      sizeof(vtkIdType)*srcG->VertexMapAllocatedSize);
  }

  if(srcG->TriangleVertexMapSize){
    this->TriangleVertexMapSize = srcG->TriangleVertexMapSize;
    this->TriangleVertexMapAllocatedSize = srcG->TriangleVertexMapAllocatedSize;
    this->TriangleVertexMap = (int *) malloc(
      sizeof(int)*this->TriangleVertexMapAllocatedSize);
    memcpy(this->TriangleVertexMap, srcG->TriangleVertexMap,
      sizeof(int)*srcG->TriangleVertexMapAllocatedSize);
  }
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkReebGraph);

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::SetLabel(vtkIdType arcId,
                                            vtkReebLabelTag Label)
{
  inputMesh = NULL;

  ResizeMainLabelTable(1);

  //create a new label in the graph
  vtkIdType L = this->MainLabelTable.FreeZone;
  this->MainLabelTable.FreeZone = this->GetLabel(L)->ArcId;
  ++(this->MainLabelTable.Number);
  memset(this->GetLabel(L),0,sizeof(vtkReebLabel));
  vtkReebLabel* l = this->GetLabel(L);

  l->HPrev=0;
  l->HNext=0;
  this->GetArc(arcId)->LabelId0=L;
  this->GetArc(arcId)->LabelId1=L;

  l->ArcId = arcId;
  l->label=Label;

  vtkIdType Lp = FindDwLabel(this->GetArc(arcId)->NodeId0,Label);
  vtkIdType Ln = FindUpLabel(this->GetArc(arcId)->NodeId1,Label);

  l->VPrev=Lp;
  if (Lp) this->GetLabel(Lp)->VNext=L;
  l->VNext=Ln;
  if (Ln) this->GetLabel(Ln)->VPrev=L;
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::FastArcSimplify(vtkIdType arcId,
                                                   int vtkNotUsed(ArcNumber),
                                                   vtkIdType* vtkNotUsed(arcTable))
{
  // Remove the arc which opens the loop
  vtkIdType nodeId0 = this->GetArc(arcId)->NodeId0;
  vtkIdType nodeId1 = this->GetArc(arcId)->NodeId1;

  vtkReebArc *A = this->GetArc(arcId);
  vtkReebArc *B = NULL;
  int down, middle, up;

  if(historyOn)
    {
    if(A->ArcDwId0)
      {
      B = this->GetArc(A->ArcDwId0);

      down = this->GetNode(B->NodeId0)->VertexId;
      middle = this->GetNode(A->NodeId0)->VertexId;
      up = this->GetNode(B->NodeId1)->VertexId;

      vtkReebCancellation c;
      c.removedArcs.push_back(std::pair<int, int>(middle, up));
      c.insertedArcs.push_back(std::pair<int, int>(down, up));
      this->cancellationHistory.push_back(c);
      }
    if(A->ArcDwId1)
      {
      B = this->GetArc(A->ArcDwId1);

      down = this->GetNode(B->NodeId0)->VertexId;
      middle = this->GetNode(A->NodeId0)->VertexId;
      up = this->GetNode(A->NodeId1)->VertexId;

      vtkReebCancellation c;
      c.removedArcs.push_back(std::pair<int, int>(middle, up));
      c.insertedArcs.push_back(std::pair<int, int>(down, up));
      this->cancellationHistory.push_back(c);
      }
    if(A->ArcUpId0)
      {
      B = this->GetArc(A->ArcUpId0);

      down = this->GetNode(A->NodeId0)->VertexId;
      middle = this->GetNode(A->NodeId1)->VertexId;
      up = this->GetNode(B->NodeId1)->VertexId;

      vtkReebCancellation c;
      c.removedArcs.push_back(std::pair<int, int>(down, middle));
      c.insertedArcs.push_back(std::pair<int, int>(down, up));
      this->cancellationHistory.push_back(c);
      }
    if(A->ArcUpId1)
      {
      B = this->GetArc(A->ArcUpId1);

      down = this->GetNode(B->NodeId0)->VertexId;
      middle = this->GetNode(A->NodeId1)->VertexId;
      up = this->GetNode(B->NodeId1)->VertexId;

      vtkReebCancellation c;
      c.removedArcs.push_back(std::pair<int, int>(down, middle));
      c.insertedArcs.push_back(std::pair<int, int>(down, up));
      this->cancellationHistory.push_back(c);
      }
    }

  vtkReebGraphRemoveUpArc(this, nodeId0, arcId);
  vtkReebGraphRemoveDownArc(this, nodeId1, arcId);

  //delete the arc from the graph...
  this->GetArc(arcId)->LabelId1 = ((int)-2);
  this->GetArc(arcId)->LabelId0 = this->MainArcTable.FreeZone;
  this->MainArcTable.FreeZone=(arcId);
  --(this->MainArcTable.Number);
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::FindGreater(vtkIdType nodeId,
                                                    vtkIdType startingNodeId,
                                                    vtkReebLabelTag label)
{
  if (!this->GetNode(nodeId)->IsFinalized)
    return 0;

  //base case
  if (vtkReebGraphIsHigherThan(this, nodeId, startingNodeId, this->GetNode(nodeId), this->GetNode(startingNodeId)))
    return nodeId;

  //iterative case
  for (vtkIdType A = this->GetNode(nodeId)->ArcUpId; A; A = this->GetArc(A)->ArcDwId0)
  {
    vtkReebArc* a = this->GetArc(A);
    vtkIdType M = this->GetArc(A)->NodeId1;
    vtkReebNode* m = this->GetNode(M);

    if (a->LabelId0 || !m->IsFinalized) //other labels or not final node
      {
      continue;
      }

    if ((M = FindGreater(M, startingNodeId, label)))
    {
      if (label)
        {
        SetLabel(A, label);
        }
      return M;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::FindLess(vtkIdType nodeId,
                                                 vtkIdType startingNodeId,
                                                 vtkReebLabelTag label)
{
  if (!this->GetNode(nodeId)->IsFinalized)
    return 0;

  //base case
  if(vtkReebGraphIsSmaller(this, nodeId, startingNodeId, this->GetNode(nodeId), this->GetNode(startingNodeId)))
    return nodeId;

  //iterative case
  for (vtkIdType A = this->GetNode(nodeId)->ArcDownId; A; A = this->GetArc(A)->ArcDwId1)
    {
    vtkReebArc* a = this->GetArc(A);
    vtkIdType M = this->GetArc(A)->NodeId0;
    vtkReebNode* m = this->GetNode(M);

    if (a->LabelId0 || !m->IsFinalized) //other labels or not final node
      continue;

    if ((M = FindLess(M, startingNodeId, label)))
      {
      if (label)
        {
        SetLabel(A, label);
        }
      return M;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::FindJoinNode(vtkIdType arcId,
                                                     vtkReebLabelTag label,
                                                     bool onePathOnly)
{
  vtkIdType N = this->GetArc(arcId)->NodeId1;
  vtkIdType Ret,C;

  if (this->GetArc(arcId)->LabelId0 || !this->GetNode(N)->IsFinalized)
    //other labels or not final node
    return 0;

  if (onePathOnly && (this->GetArc(arcId)->ArcDwId0 || this->GetArc(arcId)->ArcUpId0))
    return 0;

  //base case
  if (this->GetArc(arcId)->ArcDwId1 || this->GetArc(arcId)->ArcUpId1)
    {
    if (label) SetLabel(arcId, label);
    return N;
    }

  for (C = this->GetNode(N)->ArcUpId; C; C = this->GetArc(C)->ArcDwId0)
    {
    Ret = FindJoinNode(C, label, onePathOnly);

    if (Ret)
      {
      if (label) SetLabel(arcId, label);
      return Ret;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::FindSplitNode(vtkIdType arcId,
                                                      vtkReebLabelTag label,
                                                      bool onePathOnly)
{
  vtkIdType N = this->GetArc(arcId)->NodeId0;
  vtkIdType Ret,C;

  if (this->GetArc(arcId)->LabelId0 || !this->GetNode(N)->IsFinalized)
    //other labels or not final node
    return 0;

  if (onePathOnly && (this->GetArc(arcId)->ArcDwId1 || this->GetArc(arcId)->ArcUpId1))
    return 0;

  //base case
  if (this->GetArc(arcId)->ArcDwId0 || this->GetArc(arcId)->ArcUpId0)
    {
    if (label) SetLabel(arcId, label);
    return N;
    }

  //iterative case
  for (C = this->GetNode(N)->ArcDownId; C; C = this->GetArc(C)->ArcDwId1)
    {
    Ret = FindSplitNode(C, label, onePathOnly);

    if (Ret)
      {
      if (label) SetLabel(arcId, label);
      return Ret;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkReebGraph::Implementation::vtkReebPath vtkReebGraph::Implementation::FindPath(vtkIdType arcId,
  double simplificationThreshold, vtkReebGraphSimplificationMetric *metric)
{
  vtkReebPath entry;
  std::priority_queue<vtkReebPath> pq;
  int size;

  vtkIdType N0 = this->GetArc(arcId)->NodeId0;
  vtkIdType N1 = this->GetArc(arcId)->NodeId1;

  char* Ntouch = 0;
  char* Atouch = 0;

//  double simplificationValue = 0;
  if((!inputMesh)||(!metric))
    {
    double f0 = this->GetNode(N0)->Value;
    double f1 = this->GetNode(N1)->Value;
    entry.SimplificationValue = (f1 - f0)
      /(this->MaximumScalarValue - this->MinimumScalarValue);
    }
  else
    {
    entry.SimplificationValue = ComputeCustomMetric(metric, this->GetArc(arcId));
    }

  //the arc itself has a good persistence
  if (simplificationThreshold
    && entry.SimplificationValue>= simplificationThreshold)
    {
    NOT_FOUND:
    if (Ntouch) free(Ntouch);
    if (Atouch) free(Atouch);
    vtkReebPath fake;memset(&fake,0,sizeof(vtkReebPath));
    fake.SimplificationValue = 1;
    return fake;
    }

  Atouch=(char*)malloc(sizeof(char)*this->MainArcTable.Size);
  Ntouch=(char*)malloc(sizeof(char)*this->MainNodeTable.Size);
  memset(Atouch,0,sizeof(char)*this->MainArcTable.Size);
  memset(Ntouch,0,sizeof(char)*this->MainNodeTable.Size);

  Ntouch[N0]=1;

  //I don't want to use the arc given by the user
  Atouch[arcId]=1;

  entry.NodeNumber=1;
  entry.NodeTable = new vtkIdType[1];
  entry.NodeTable[0]=N0;
  entry.ArcNumber=0;
  entry.ArcTable =0;
  pq.push(entry);

  while ((size=static_cast<int>(pq.size())))
    {
    entry=pq.top();
    pq.pop();

    int N=entry.NodeTable[entry.NodeNumber-1];

    for (int dir=0;dir<=1;dir++)
      {
      for (int A = (!dir) ? this->GetNode(N)->ArcDownId : this->GetNode(N)->ArcUpId; A;
           A=(!dir) ? this->GetArc(A)->ArcDwId1 : this->GetArc(A)->ArcDwId0)
        {
        int M = (!dir) ? (this->GetArc(A)->NodeId0) : (this->GetArc(A)->NodeId1);

        if (Atouch[A]) continue;
        Atouch[A]=1;

        //already used (==there is a better path to reach M)
        if (Ntouch[M]) continue;
        Ntouch[M]=1;

        //found!!!
        if (M==N1)
        {
          //clear all the items in the priority queue
          while (pq.size())
          {
            vtkReebPath aux=pq.top();pq.pop();
            delete aux.ArcTable;
            delete aux.NodeTable;
          }

          if (Ntouch) free(Ntouch);
          if (Atouch) free(Atouch);

          vtkIdType* tmp=new vtkIdType[entry.NodeNumber+1];
          memcpy(tmp,entry.NodeTable,sizeof(vtkIdType)*entry.NodeNumber);
          tmp[entry.NodeNumber]=N1;
          delete [] entry.NodeTable;
          entry.NodeTable = tmp;
          entry.NodeNumber++;
          return entry;
        }

        if((!inputMesh)||(!metric))
          {
          entry.SimplificationValue += vtkReebGraphGetArcPersistence(this, this->GetArc(A));
          }
        else
          {
          entry.SimplificationValue += ComputeCustomMetric(metric, this->GetArc(A));
          }

        // The loop persistence is greater than functionScale
        if(simplificationThreshold
          && entry.SimplificationValue >= simplificationThreshold)
          continue;

        vtkReebPath newentry;
        newentry.SimplificationValue = entry.SimplificationValue;
        newentry.ArcNumber = entry.ArcNumber+1;
        newentry.ArcTable = new vtkIdType [newentry.ArcNumber];
        newentry.NodeNumber = entry.NodeNumber+1;
        newentry.NodeTable = new vtkIdType [newentry.NodeNumber];
        if (entry.ArcNumber)
          memcpy(newentry.ArcTable,entry.ArcTable,
            sizeof(vtkIdType)*entry.ArcNumber);
        if (entry.NodeNumber)
          memcpy(newentry.NodeTable,entry.NodeTable,
            sizeof(vtkIdType)*entry.NodeNumber);

        newentry.ArcTable[entry.ArcNumber]=A;
        newentry.NodeTable[entry.NodeNumber]=M;
        pq.push(newentry);
        }
      }

    //finished with this entry
    delete entry.ArcTable;
    delete [] entry.NodeTable;
    }

  goto NOT_FOUND;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::SimplifyLoops(double simplificationThreshold,
                                                vtkReebGraphSimplificationMetric *simplificationMetric)
{

 if (!simplificationThreshold)
   return 0;

 // refresh information about ArcLoopTable
 this->FindLoops();

 int NumSimplified=0;

 for (int n = 0; n < this->LoopNumber; n++)
 {
   int A = this->ArcLoopTable[n];

   if (this->GetArc(A)->LabelId1 == ((int)-2))
     continue;

   double simplificationValue = 0;
   if((!inputMesh)||(!simplificationMetric))
    {
    vtkIdType N0 = this->GetArc(A)->NodeId0;
    vtkIdType N1 = this->GetArc(A)->NodeId1;
    double f0 = this->GetNode(N0)->Value;
    double f1 = this->GetNode(N1)->Value;
    simplificationValue = (f1 - f0)
      /(this->MaximumScalarValue - this->MinimumScalarValue);
    }
   else
    {
    simplificationValue = ComputeCustomMetric(simplificationMetric, this->GetArc(A));
    }


   if(simplificationValue >= simplificationThreshold)
     continue;

   vtkReebPath entry = this->FindPath(this->ArcLoopTable[n],
    simplificationThreshold, simplificationMetric);

   //too high for persistence
   if(entry.SimplificationValue >= simplificationThreshold)
     continue;

   //distribute its bucket to the loop and delete the arc
   this->FastArcSimplify(ArcLoopTable[n],entry.ArcNumber,entry.ArcTable);
   delete entry.ArcTable;
   delete entry.NodeTable;

   ++NumSimplified;
   CommitSimplification();
 }

 //check for regular points
 for (int N=1;N<this->MainNodeTable.Size;N++)
   {
   if(this->GetNode(N)->ArcUpId == ((int)-2))
     continue;

   if (this->GetNode(N)->ArcDownId==0 && this->GetNode(N)->ArcUpId==0)
     {
     //delete the node from the graph...
     this->GetNode(N)->ArcUpId = ((int)-2);
     this->GetNode(N)->ArcDownId = this->MainNodeTable.FreeZone;
     this->MainNodeTable.FreeZone=(N);
     --(this->MainNodeTable.Number);

     }

   else if (vtkReebGraphIsRegular(this,this->GetNode(N)))
     {
     if(historyOn)
       {
       vtkReebNode *n = this->GetNode(N);

       int A0 = n->ArcDownId;
       int A1 = n->ArcUpId;

       vtkReebArc *a0 = this->GetArc(A0);
       vtkReebArc *a1 = this->GetArc(A1);
       vtkReebNode *downN = this->GetNode(a0->NodeId0);
       vtkReebNode *upN = this->GetNode(a1->NodeId1);

       int down, middle, up;
       down = downN->VertexId;
       middle = n->VertexId;
       up = upN->VertexId;

       vtkReebCancellation c;
       c.removedArcs.push_back(std::pair<int, int>(down, middle));
       c.removedArcs.push_back(std::pair<int, int>(middle, up));
       c.insertedArcs.push_back(std::pair<int, int>(down, up));

       this->cancellationHistory.push_back(c);
       }
     EndVertex(N);
     }
   }

 this->RemovedLoopNumber = NumSimplified;

 return NumSimplified;
}

//----------------------------------------------------------------------------
double vtkReebGraph::Implementation::ComputeCustomMetric(
  vtkReebGraphSimplificationMetric *simplificationMetric, vtkReebArc *a)
{
  int edgeId = -1, start = -1, end = -1;

  vtkDataArray *vertexInfo = vtkDataArray::SafeDownCast(
    this->Parent->GetVertexData()->GetAbstractArray("Vertex Ids"));
  if(!vertexInfo) return vtkReebGraphGetArcPersistence(this, a);

  vtkVariantArray *edgeInfo = vtkVariantArray::SafeDownCast(
    this->Parent->GetEdgeData()->GetAbstractArray("Vertex Ids"));
  if(!edgeInfo) return vtkReebGraphGetArcPersistence(this, a);

  vtkEdgeListIterator *eIt = vtkEdgeListIterator::New();
  this->Parent->GetEdges(eIt);

  do
    {
    vtkEdgeType e = eIt->Next();
    if(((*(vertexInfo->GetTuple(e.Source))) == GetNodeVertexId(a->NodeId0))
       &&((*(vertexInfo->GetTuple(e.Target))) == GetNodeVertexId(a->NodeId1)))
      {
      edgeId = e.Id;
      start = static_cast<int>(*(vertexInfo->GetTuple(e.Source)));
      end = static_cast<int>(*(vertexInfo->GetTuple(e.Target)));
      break;
      }
    }while(eIt->HasNext());
  eIt->Delete();

  vtkAbstractArray *vertexList = edgeInfo->GetPointer(edgeId)->ToArray();

  return simplificationMetric->ComputeMetric(inputMesh, inputScalarField,
    start, vertexList, end);
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::SimplifyBranches(double simplificationThreshold,
                                                   vtkReebGraphSimplificationMetric *simplificationMetric)
{
  static const vtkReebLabelTag RouteOld=100;
  static const vtkReebLabelTag RouteNew=200;
  int  nstack,mstack=0;
  int* stack=0;

  if (!simplificationThreshold)
    return 0;

  int nsimp=0;
  int cont=0;
  const int step=10000;
  bool redo;

  vtkDataSet *input = inputMesh;

  REDO:
  nstack=0;
  redo=false;

  for (int N = 1; N < this->MainNodeTable.Size; ++N)
    {
    if(this->GetNode(N)->ArcUpId == ((int)-2))
      continue;

    vtkReebNode* n = this->GetNode(N);

    //simplify atomic nodes
    if (!n->ArcDownId && !n->ArcUpId)
      {
      //delete the node from the graph...
      this->GetNode(N)->ArcUpId = ((int)-2);
      this->GetNode(N)->ArcDownId = this->MainNodeTable.FreeZone;
      this->MainNodeTable.FreeZone=(N);
      --(this->MainNodeTable.Number);

      }
    else if (!n->ArcDownId)
      {
      //insert into stack branches to simplify
      for (int _A_ = n->ArcUpId; _A_; _A_ = this->GetArc(_A_)->ArcDwId0)
        {
        vtkReebArc* _a_ = this->GetArc(_A_);
        if((!inputMesh)||(!simplificationMetric))
          {
          if (vtkReebGraphGetArcPersistence(this,_a_)< simplificationThreshold)
            {
            vtkReebGraphStackPush(_A_);
            }
          }
        else
          {
          if(this->ComputeCustomMetric(simplificationMetric, _a_)
            < simplificationThreshold)
            {
            vtkReebGraphStackPush(_A_);
            }
          }
        }
      }
    else if (!n->ArcUpId)
      {
      //insert into stack branches to simplify
      for (int _A_=n->ArcDownId;_A_;_A_=this->GetArc(_A_)->ArcDwId1)
        {
        vtkReebArc* _a_ = this->GetArc(_A_);
        if (vtkReebGraphGetArcPersistence(this,_a_)< simplificationThreshold)
          {
          vtkReebGraphStackPush(_A_);
          }
        }
      }
    }

  while (vtkReebGraphStackSize())
    {
    int A=vtkReebGraphStackTop();vtkReebGraphStackPop();

    if (!--cont)
      {
      cont=step;
      }

    if (this->GetArc(A)->LabelId1 == ((int)-2))
      continue;

    cont++;

    vtkReebArc* arc = this->GetArc(A);

    int N = arc->NodeId0;
    int M = arc->NodeId1;

    if (this->GetNode(N)->ArcDownId && this->GetNode(M)->ArcUpId)
      continue;

    double persistence = vtkReebGraphGetArcPersistence(this, arc);

    //is the actual persistence (in percentage) greater than the applied filter?
    if (persistence>=simplificationThreshold)
      continue;

    int _A, Mdown = 0, Nup = 0, Ndown = 0, Mup = 0;

    //get the 'down' degree for M
    for(_A = this->GetNode(M)->ArcDownId;_A;_A = this->GetArc(_A)->ArcDwId1)
      ++Mdown;

    //Get the 'up' degree for N
    for(_A = this->GetNode(N)->ArcUpId;_A;_A = this->GetArc(_A)->ArcDwId0)
      ++Nup;

    //get the 'down' degree for N
    for(_A = this->GetNode(N)->ArcDownId;_A;_A = this->GetArc(_A)->ArcDwId1)
      ++Ndown;

    //get the 'up' degree for M
    for(_A = this->GetNode(M)->ArcUpId;_A;_A = this->GetArc(_A)->ArcDwId0)
      ++Mup;

    //isolated arc
    if (!Ndown && Nup==1 && Mdown==1 && !Mup)
      {
      vtkReebGraphRemoveUpArc  (this,N,A);
      vtkReebGraphRemoveDownArc(this,M,A);

      //delete the arc from the graph...
      this->GetArc(A)->LabelId1 = ((int)-2);
      this->GetArc(A)->LabelId0 = this->MainArcTable.FreeZone;
      this->MainArcTable.FreeZone=(A);
      --(this->MainArcTable.Number);

      if (!(this->GetNode(N)->ArcUpId == ((int)-2)) &&
          vtkReebGraphIsRegular(this, this->GetNode(N)))
        {
        EndVertex(N);
        }
      if (!(this->GetNode(M)->ArcUpId == ((int)-2)) &&
          vtkReebGraphIsRegular(this, this->GetNode(M)))
        {
        EndVertex(M);
        }

      nsimp++;redo=true;
      continue;
      }

    int Down=0;
    int Up=0;

    bool simplified=false;

    // M is a maximum
    if (!simplified && !Mup)
      {
      if ((Down = FindSplitNode(A, RouteOld)))
        {
        if ((Up = FindGreater(Down, M, RouteNew)))
          {
          SetLabel(AddArc(M,Up),RouteOld);
          Collapse(Down,Up,RouteOld,RouteNew);
          simplified=true;
          }
        else
          {
          this->SimplifyLabels(Down);
          }
        }
      }

    //N is a minimum
    if (!simplified && !Ndown)
      {
      if ((Up = FindJoinNode(A, RouteOld)))
        {
        if ((Down = FindLess(Up, N, RouteNew)))
          {
          SetLabel(AddArc(Down,N),RouteOld);
          Collapse(Down,Up,RouteOld,RouteNew);
          simplified=true;
          }
        else
          {
          this->SimplifyLabels(Up);
          }
        }
      }

    if (simplified)
      {
      if (!(this->GetNode(Down)->ArcUpId == ((int)-2)))
        {
        this->SimplifyLabels(Down);

        if (!this->GetNode(Down)->ArcDownId) //minimum
          {
          for (vtkIdType _A_ = this->GetNode(Down)->ArcUpId; _A_; _A_ = this->GetArc(_A_)->ArcDwId0)
            {
            vtkReebArc* _a_ = this->GetArc(_A_);
            if((!inputMesh)||(!simplificationMetric))
              {
              if (vtkReebGraphGetArcPersistence(this,_a_)
                  < simplificationThreshold)
                {
                vtkReebGraphStackPush(_A_);
                }
              }
            else
              {
              if(this->ComputeCustomMetric(simplificationMetric, _a_) < simplificationThreshold)
                {
                vtkReebGraphStackPush(_A_);
                }
              }
            }
          }
        }

    if (!(this->GetNode(Up)->ArcUpId == ((int)-2)))
        {
        this->SimplifyLabels(Up);

        if (!this->GetNode(Up)->ArcUpId)
          {
          for (int _A_ = this->GetNode(Up)->ArcDownId; _A_; _A_ = this->GetArc(_A_)->ArcDwId1)
            {
            vtkReebArc* _a_ = this->GetArc(_A_);
            if((!inputMesh)||(!simplificationMetric))
              {
              if (vtkReebGraphGetArcPersistence(this,_a_)
                < simplificationThreshold)
                {
                vtkReebGraphStackPush(_A_);
                }
              }
            else
              {
              if(this->ComputeCustomMetric(simplificationMetric, _a_)
                < simplificationThreshold)
                {
                vtkReebGraphStackPush(_A_);
                }
              }
            }
          }
        }

      nsimp++;
      redo=true;
      }
    CommitSimplification();
    } //while

  if (redo)
    goto REDO;

  free(stack);

  inputMesh = input;

  return nsimp;
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::ResizeMainNodeTable(int newSize)
{
  int oldsize,i;

  if ((this->MainNodeTable.Size-this->MainNodeTable.Number)< newSize)
    {
    oldsize=this->MainNodeTable.Size;

    if (!this->MainNodeTable.Size) this->MainNodeTable.Size = newSize;
    while ((this->MainNodeTable.Size-this->MainNodeTable.Number)< newSize)
      this->MainNodeTable.Size<<=1;

    this->MainNodeTable.Buffer = (vtkReebNode*)realloc(
      this->MainNodeTable.Buffer,sizeof(vtkReebNode)*this->MainNodeTable.Size);

    for (i=oldsize;i<this->MainNodeTable.Size-1;i++)
      {
      this->GetNode(i)->ArcDownId=i+1;
      this->GetNode(i)->ArcUpId = ((int)-2);
      }

    this->GetNode(i)->ArcDownId = this->MainNodeTable.FreeZone;
    this->GetNode(i)->ArcUpId = ((int)-2);
    this->MainNodeTable.FreeZone=oldsize;
    }
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::CommitSimplification()
{
  // now re-construct the graph with projected deg-2 nodes.
  std::vector<std::pair<std::pair<int, int>, std::vector<int> > > before, after;

  vtkEdgeListIterator *eIt = vtkEdgeListIterator::New();
  this->Parent->GetEdges(eIt);
  vtkVariantArray *edgeInfo = vtkVariantArray::SafeDownCast(
    this->Parent->GetEdgeData()->GetAbstractArray("Vertex Ids"));
  vtkDataArray    *vertexInfo =
      this->Parent->GetVertexData()->GetArray("Vertex Ids");

  // avoids double projection
  int vertexNumber = vertexInfo->GetNumberOfTuples();
  std::vector<bool> segmentedVertices;

  do
    {
    std::pair<std::pair<int, int>, std::vector<int> > superArc;

    vtkEdgeType e = eIt->Next();
    vtkAbstractArray *vertexList = edgeInfo->GetPointer(e.Id)->ToArray();

    superArc.first.first = (int) *(vertexInfo->GetTuple(e.Source));
    superArc.first.second = (int) *(vertexInfo->GetTuple(e.Target));

    superArc.second.resize(vertexList->GetNumberOfTuples());
    vertexNumber += vertexList->GetNumberOfTuples();
    for(unsigned int i = 0; i < superArc.second.size(); i++)
      superArc.second[i] = vertexList->GetVariantValue(i).ToInt();

    before.push_back(superArc);
    }while(eIt->HasNext());

  segmentedVertices.resize(vertexNumber);
  for(unsigned int i = 0; i < segmentedVertices.size(); i++)
    segmentedVertices[i] = false;

  vtkIdType prevArcId = -1, arcId = 0;
  while(arcId != prevArcId)
    {
    prevArcId = arcId;
    arcId = GetPreviousArcId();
    }
  prevArcId = -1;


  while(prevArcId != arcId)
    {
    if(this->GetArc(arcId))
      {
      int down, up;
      down = this->GetNode((this->GetArc(arcId))->NodeId0)->VertexId;
      up = this->GetNode((this->GetArc(arcId))->NodeId1)->VertexId;

      std::pair<std::pair<int, int>, std::vector<int> > superArc;

      superArc.first.first = down;
      superArc.first.second = up;

      after.push_back(superArc);
      }
    prevArcId = arcId;
    arcId = GetNextArcId();
    }

  std::pair<int, int> destinationArc;
  std::map<int, bool> processedInputArcs, processedOutputArcs;

  // now map the unsimplified arcs onto the simplified ones
  for(unsigned int i = 0; i < before.size(); i++)
    {
    std::vector<int> simplifiedCriticalNodes;
    destinationArc = before[i].first;
    for(unsigned int j = 0; j < this->cancellationHistory.size(); j++)
      {
      for(unsigned int k = 0;
          k < this->cancellationHistory[j].removedArcs.size(); k++)
        {
        if((destinationArc.first ==
            this->cancellationHistory[j].removedArcs[k].first)
          &&(destinationArc.second
            == this->cancellationHistory[j].removedArcs[k].second))
          {
          // the arc has been involved in a cancellation
          destinationArc = this->cancellationHistory[j].insertedArcs[0];

          if(this->cancellationHistory[j].removedArcs.size() > 1)
            {
            if(((this->cancellationHistory[j].removedArcs[0].first
              == destinationArc.first)
              &&(this->cancellationHistory[j].removedArcs[1].second
                == destinationArc.second))
              ||
              ((this->cancellationHistory[j].removedArcs[1].first
                == destinationArc.first)
              &&(this->cancellationHistory[j].removedArcs[0].second
                == destinationArc.second)))
              {
              for(unsigned int l = 0;
                  l < this->cancellationHistory[j].removedArcs.size(); l++)
                {
                if((this->cancellationHistory[j].removedArcs[l].first
                  != destinationArc.first)
                  &&(this->cancellationHistory[j].removedArcs[l].first
                  != destinationArc.second))
                  {
                  // this critical node will become a degree two node, let's
                  // remember it
                  simplifiedCriticalNodes.push_back(
                    this->cancellationHistory[j].removedArcs[l].first);
                  }
                if((this->cancellationHistory[j].removedArcs[l].second
                  != destinationArc.first)
                  &&(this->cancellationHistory[j].removedArcs[l].second
                  != destinationArc.second))
                  {
                  // same thing as above
                  simplifiedCriticalNodes.push_back(
                    this->cancellationHistory[j].removedArcs[l].second);
                  }
                }
              }
            }
          }
        }
      }

      // at this point the deg2-nodes are in before[i].second

      // now find the projection in the simplified graph
      for(unsigned int j = 0; j < after.size(); j++)
        {
        if(destinationArc == after[j].first)
          {
          std::map<int, bool>::iterator aIt;
          aIt = processedOutputArcs.find(j);
          if(aIt == processedOutputArcs.end())
            {
            if(before[i].first == destinationArc)
              {
              // non-simplified arc
              processedOutputArcs[j] = true;
              after[j].second = before[i].second;
              }
            if(before[i].first != destinationArc)
              {
              // adding content of before[i].second to after[j].second
              for(unsigned int k = 0; k < before[i].second.size(); k++)
                {
                if(!segmentedVertices[before[i].second[k]])
                  {
                  after[j].second.push_back(before[i].second[k]);
                  segmentedVertices[before[i].second[k]] = true;
                  }
                }
              }
              for(unsigned int k = 0; k < simplifiedCriticalNodes.size(); k++)
                {
                if(!segmentedVertices[simplifiedCriticalNodes[k]])
                  {
                  after[j].second.push_back(simplifiedCriticalNodes[k]);
                  segmentedVertices[simplifiedCriticalNodes[k]] = true;
                  }
                }
              break;
            }
          }
        }
    }
  // ensure the sorting on the arcs
  for(unsigned int i = 0; i < after.size(); i++)
    {
    std::vector<std::pair<int, double> > scalarValues;
    for(unsigned int j = 0; j < after[i].second.size(); j++)
      {
      std::pair<int, double> scalarVertex;
      scalarVertex.first = after[i].second[j];
      std::map<int, double>::iterator sIt;
      sIt = ScalarField.find(scalarVertex.first);
      if(sIt != ScalarField.end())
        {
        scalarVertex.second = sIt->second;
        scalarValues.push_back(scalarVertex);
        }
      }
    std::sort(scalarValues.begin(), scalarValues.end(), vtkReebGraphVertexSoS);
    for(unsigned int j = 0; j < after[i].second.size(); j++)
      after[i].second[j] = scalarValues[j].first;
    }

  // now construct the vtkMutableDirectedGraph
  // first, clean up the current graph
  while(this->Parent->GetNumberOfEdges()) this->Parent->RemoveEdge(0);
  while(this->Parent->GetNumberOfVertices()) this->Parent->RemoveVertex(0);

  this->Parent->GetVertexData()->RemoveArray("Vertex Ids");
  this->Parent->GetEdgeData()->RemoveArray("Vertex Ids");

  vtkIdType prevNodeId = -1, nodeId = 0;
  while(prevNodeId != nodeId)
    {
    prevNodeId = nodeId;
    nodeId = GetPreviousNodeId();
    }
  prevNodeId = -1;

  vtkVariantArray *vertexProperties = vtkVariantArray::New();
  vertexProperties->SetNumberOfValues(1);

  vtkIdTypeArray *vertexIds = vtkIdTypeArray::New();
  vertexIds->SetName("Vertex Ids");
  this->Parent->GetVertexData()->AddArray(vertexIds);

  std::map<int, int> vMap;
  int vIt = 0;
  while(prevNodeId != nodeId)
    {
    vtkIdType nodeVertexId = GetNodeVertexId(nodeId);
    vMap[nodeVertexId] = vIt;
    vertexProperties->SetValue(0, nodeVertexId);
    this->Parent->AddVertex(vertexProperties);

    prevNodeId = nodeId;
    nodeId = GetNextNodeId();
    vIt++;
    }
  vertexIds->Delete();
  vertexProperties->Delete();

  vtkVariantArray *deg2NodeIds = vtkVariantArray::New();
  deg2NodeIds->SetName("Vertex Ids");
  this->Parent->GetEdgeData()->AddArray(deg2NodeIds);

  for(unsigned int i = 0; i < after.size(); i++)
    {
    std::map<int, int>::iterator downIt, upIt;
    downIt = vMap.find(after[i].first.first);
    upIt = vMap.find(after[i].first.second);

    if((downIt != vMap.end())&&(upIt != vMap.end()))
      {
      vtkVariantArray *edgeProperties = vtkVariantArray::New();
      vtkIdTypeArray  *vertexList = vtkIdTypeArray::New();
      vertexList->SetNumberOfValues(after[i].second.size());
      for(unsigned int j = 0; j < after[i].second.size(); j++)
        vertexList->SetValue(j, after[i].second[j]);
      edgeProperties->SetNumberOfValues(1);
      edgeProperties->SetValue(0, vertexList);
      this->Parent->AddEdge(downIt->second, upIt->second, edgeProperties);
      vertexList->Delete();
      edgeProperties->Delete();
      }
    }
  deg2NodeIds->Delete();

  this->cancellationHistory.clear();

  return 0;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Simplify(double simplificationThreshold,
  vtkReebGraphSimplificationMetric *simplificationMetric)
{
  int deletionNumber = 0;

  this->Storage->cancellationHistory.clear();
  this->Storage->historyOn = true;

  this->Storage->ArcNumber = 0;
  this->Storage->NodeNumber = 0;

  deletionNumber =
    this->Storage->SimplifyBranches(
      simplificationThreshold, simplificationMetric)
    + this->Storage->SimplifyLoops(
      simplificationThreshold, simplificationMetric)
    + this->Storage->SimplifyBranches(
      simplificationThreshold, simplificationMetric);

  this->Storage->historyOn = false;

  return deletionNumber;
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::FlushLabels()
{
  for (int A=1;A<this->MainArcTable.Size;A++)
    {
    if (!(this->GetArc(A)->LabelId1 == ((int)-2)))
      this->GetArc(A)->LabelId0 = this->GetArc(A)->LabelId1=0;
    }

  if (this->MainLabelTable.Buffer)
    {
    free(this->MainLabelTable.Buffer);
    }

  this->MainLabelTable.Buffer=(vtkReebLabel*)malloc(sizeof(vtkReebLabel)*2);
  this->MainLabelTable.Size=2;
  this->MainLabelTable.Number=1;
  this->MainLabelTable.FreeZone=1;
  this->GetLabel(1)->HNext = ((int)-2);
  this->GetLabel(1)->ArcId = 0;
}

//----------------------------------------------------------------------------
void vtkReebGraph::DeepCopy(vtkDataObject *src)
{

  vtkReebGraph *srcG = vtkReebGraph::SafeDownCast(src);

  if (srcG)
    {
    this->Storage->DeepCopy(srcG->Storage);
    }

  vtkMutableDirectedGraph::DeepCopy(srcG);
}

//----------------------------------------------------------------------------
void vtkReebGraph::Set(vtkMutableDirectedGraph *g)
{
  vtkMutableDirectedGraph::DeepCopy(g);
}

//----------------------------------------------------------------------------
void vtkReebGraph::CloseStream()
{

  vtkIdType prevArcId = -1, arcId = 0;
  while(arcId != prevArcId)
    {
    prevArcId = arcId;
    arcId = this->Storage->GetPreviousArcId();
    }
  prevArcId = -1;

  // loop over the arcs and build the local adjacency map

  // vertex -> (down vertices, up vertices)
  std::map<int, std::pair<std::vector<int>, std::vector<int> > > localAdjacency;
  while(prevArcId != arcId)
    {
    vtkIdType downVertexId, upVertexId;
    downVertexId = this->Storage->GetNode((this->Storage->GetArc(arcId))->NodeId0)->VertexId;
    upVertexId = this->Storage->GetNode((this->Storage->GetArc(arcId))->NodeId1)->VertexId;

    std::map<int,
      std::pair<std::vector<int>, std::vector<int> > >::iterator aIt;

    // lookUp for the down vertex
    aIt = localAdjacency.find(downVertexId);
    if(aIt == localAdjacency.end())
      {
      std::pair<std::vector<int>, std::vector<int> > adjacencyItem;
      adjacencyItem.second.push_back(upVertexId);
      localAdjacency[downVertexId] = adjacencyItem;
      }
    else
      {
      aIt->second.second.push_back(upVertexId);
      }

    // same thing for the up vertex
    aIt = localAdjacency.find(upVertexId);
    if(aIt == localAdjacency.end())
      {
      std::pair<std::vector<int>, std::vector<int> > adjacencyItem;
      adjacencyItem.first.push_back(downVertexId);
      localAdjacency[upVertexId] = adjacencyItem;
      }
    else
      {
      aIt->second.first.push_back(downVertexId);
      }

    prevArcId = arcId;
    arcId = this->Storage->GetNextArcId();
    }

  // now build the super-arcs with deg-2 nodes

  // <vertex,vertex>,<vertex list> (arc, deg2 node list)
  std::vector<std::pair<std::pair<int, int>, std::vector<int> > >
    globalAdjacency;

  std::map<int, std::pair<std::vector<int>, std::vector<int> > >::iterator aIt;
  aIt = localAdjacency.begin();
  do
    {
    if(!((aIt->second.first.size() == 1)&&(aIt->second.second.size() == 1)))
      {
      // not a deg-2 node
      if(aIt->second.second.size())
        {
        // start the sweep up
        for(unsigned int i = 0; i < aIt->second.second.size(); i++)
          {
          std::vector<int> deg2List;
          std::map<int,
            std::pair<std::vector<int>, std::vector<int> > >::iterator nextIt;

          nextIt = localAdjacency.find(aIt->second.second[i]);
          while((nextIt->second.first.size() == 1)
            &&(nextIt->second.second.size() == 1))
            {
            deg2List.push_back(nextIt->first);
            nextIt = localAdjacency.find(nextIt->second.second[0]);
            }
            globalAdjacency.push_back(
              std::pair<std::pair<int, int>,
                std::vector<int> >(std::pair<int, int>(
                  aIt->first, nextIt->first), deg2List));
          }
        }
      }
      aIt++;
    }
  while(aIt != localAdjacency.end());

  // now cleanup the internal representation
  int nmyend=0;
  for (vtkIdType N=1;N<this->Storage->MainNodeTable.Size;N++)
    {
    // clear the node
    if (this->Storage->GetNode(N)->ArcUpId == ((int)-2))
      continue;

    vtkReebGraph::Implementation::vtkReebNode* n = this->Storage->GetNode(N);

    if (!n->IsFinalized)
      {
      nmyend++;
      this->Storage->EndVertex(N);
      }
    }

  this->Storage->FlushLabels();


  // now construct the actual graph
  vtkIdType prevNodeId = -1, nodeId = 0;
  while(prevNodeId != nodeId)
    {
    prevNodeId = nodeId;
    nodeId = this->Storage->GetPreviousNodeId();
    }
  prevNodeId = -1;

  vtkVariantArray *vertexProperties = vtkVariantArray::New();
  vertexProperties->SetNumberOfValues(1);

  vtkIdTypeArray  *vertexIds = vtkIdTypeArray::New();
  vertexIds->SetName("Vertex Ids");
  GetVertexData()->AddArray(vertexIds);

  std::map<int, int> vMap;
  int vIt = 0;

  while(prevNodeId != nodeId)
    {
    vtkIdType nodeVertexId = this->Storage->GetNodeVertexId(nodeId);
    vMap[nodeVertexId] = vIt;
    vertexProperties->SetValue(0, nodeVertexId);
    AddVertex(vertexProperties);

    prevNodeId = nodeId;
    nodeId = this->Storage->GetNextNodeId();
    vIt++;
    }
  vertexIds->Delete();
  vertexProperties->Delete();

  vtkVariantArray  *deg2NodeIds = vtkVariantArray::New();
  deg2NodeIds->SetName("Vertex Ids");
  GetEdgeData()->AddArray(deg2NodeIds);

  for(unsigned int i = 0; i < globalAdjacency.size(); i++)
    {
    std::map<int, int>::iterator downIt, upIt;
    downIt = vMap.find(globalAdjacency[i].first.first);
    upIt = vMap.find(globalAdjacency[i].first.second);

    if((downIt != vMap.end())&&(upIt != vMap.end()))
      {
      vtkVariantArray *edgeProperties = vtkVariantArray::New();
      vtkIdTypeArray  *vertexList = vtkIdTypeArray::New();
      vertexList->SetNumberOfValues(globalAdjacency[i].second.size());
      for(unsigned int j = 0; j < globalAdjacency[i].second.size(); j++)
        vertexList->SetValue(j, globalAdjacency[i].second[j]);
      edgeProperties->SetNumberOfValues(1);
      edgeProperties->SetValue(0, vertexList);
      AddEdge(downIt->second, upIt->second, edgeProperties);
      vertexList->Delete();
      edgeProperties->Delete();
      }
    }
  deg2NodeIds->Delete();
}

//----------------------------------------------------------------------------
vtkReebGraph::vtkReebGraph()
{
  this->Storage = new vtkReebGraph::Implementation;
  this->Storage->Parent = this;
}

//----------------------------------------------------------------------------
vtkReebGraph::~vtkReebGraph()
{
  delete this->Storage;
}

//----------------------------------------------------------------------------
void vtkReebGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
  os << indent << "Reeb graph general statistics:" << endl;
  os << indent << indent << "Number Of Node(s): "
    << this->Storage->GetNumberOfNodes() << endl;
  os << indent << indent << "Number Of Arc(s): "
    << this->Storage->GetNumberOfArcs() << endl;
  os << indent << indent << "Number Of Connected Component(s): "
    << this->Storage->GetNumberOfConnectedComponents() << endl;
  os << indent << indent << "Number Of Loop(s): "
    << this->Storage->GetNumberOfLoops() << endl;
}

void vtkReebGraph::PrintNodeData(ostream& os, vtkIndent indent)
{
  vtkIdType arcId = 0, nodeId = 0;
  os << indent << "Node Data:" << endl;
  vtkIdType prevNodeId = -1;

  // roll back to the beginning of the list
  while(prevNodeId != nodeId){
    prevNodeId = nodeId;
    nodeId = this->Storage->GetPreviousNodeId();
  }
  prevNodeId = -1;


  while(prevNodeId != nodeId)
  {
    prevNodeId = nodeId;
    vtkIdList *downArcIdList = vtkIdList::New();
    vtkIdList *upArcIdList = vtkIdList::New();

    this->Storage->GetNodeDownArcIds(nodeId, downArcIdList);
    this->Storage->GetNodeUpArcIds(nodeId, upArcIdList);

    cout << indent << indent << "Node " << nodeId << ":" << endl;
    cout << indent << indent << indent;
    cout << "Vert: " << this->Storage->GetNodeVertexId(nodeId);
    cout << ", Val: " << this->Storage->GetNodeScalarValue(nodeId);
    cout << ", DwA:";
    for(vtkIdType i = 0; i < downArcIdList->GetNumberOfIds(); i++)
      cout << " " << this->Storage->GetArcDownNodeId(downArcIdList->GetId(i));
    cout << ", UpA:";
    for(vtkIdType i = 0; i < upArcIdList->GetNumberOfIds(); i++)
      cout << " " << this->Storage->GetArcUpNodeId(upArcIdList->GetId(i));
    cout << endl;

    downArcIdList->Delete();
    upArcIdList->Delete();
    nodeId = this->Storage->GetNextNodeId();
  }

  os << indent << "Arc Data:" << endl;
  vtkIdType prevArcId = -1;
  arcId = 0;

  // roll back to the beginning of the list
  while(prevArcId != arcId){
    prevArcId = arcId;
    arcId = this->Storage->GetPreviousArcId();
  }
  prevArcId = -1;

  while(prevArcId != arcId)
  {
    prevArcId = arcId;
    cout << indent << indent << "Arc " << arcId << ":" << endl;
    cout << indent << indent << indent;
    cout << "Down: " << this->Storage->GetArcDownNodeId(arcId);
    cout << ", Up: " << this->Storage->GetArcUpNodeId(arcId);
    cout << ", Persistence: "
      << this->Storage->GetNodeScalarValue(this->Storage->GetArcUpNodeId(arcId))
        - this->Storage->GetNodeScalarValue(
           this->Storage->GetArcDownNodeId(arcId));
    cout << endl;
    arcId = this->Storage->GetNextArcId();
  }
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::GetNodeDownArcIds(vtkIdType nodeId,
                                                     vtkIdList *arcIdList)
{
  vtkIdType i  = 0;

  if(!arcIdList) return;

  arcIdList->Reset();

  for(vtkIdType arcId = this->GetNode(nodeId)->ArcDownId;
    arcId; arcId = this->GetArc(arcId)->ArcDwId1)
    {
    arcIdList->InsertId(i, arcId);
    i++;
    }
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::GetNodeUpArcIds(vtkIdType nodeId,
                                                   vtkIdList *arcIdList)
{
  vtkIdType i  = 0;

  if(!arcIdList) return;

  for(vtkIdType arcId = this->GetNode(nodeId)->ArcUpId;
    arcId; arcId = this->GetArc(arcId)->ArcDwId0)
    {
    arcIdList->InsertId(i, arcId);
    i++;
    }
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::FindLoops()
{

  if (this->ArcLoopTable)
    {
    free(this->ArcLoopTable);
    this->ArcLoopTable=0;
    this->LoopNumber=0;
    }

  this->ConnectedComponentNumber=0;

  int  nstack=0,mstack=0;
  int* stack=0;

  char* Ntouch=(char*)malloc(sizeof(char)*this->MainNodeTable.Size);
  char* Atouch=(char*)malloc(sizeof(char)*this->MainArcTable.Size);

  memset(Ntouch,0,sizeof(char)*this->MainNodeTable.Size);

  for(int Node=1;Node<this->MainNodeTable.Size;Node++)
    {
    // check that the node is clear
    if (this->GetNode(Node)->ArcUpId == ((int)-2))
      continue;

    if (!Ntouch[Node])
      {
      ++(this->ConnectedComponentNumber);

      memset(Atouch,0,sizeof(bool)*this->MainArcTable.Size);

      Ntouch[Node]=1;
      nstack=0;
      vtkReebGraphStackPush(Node);

      while (vtkReebGraphStackSize())
        {

        int N=vtkReebGraphStackTop();
        vtkReebGraphStackPop();

        for (int dir=0;dir<=1;dir++)
          {
          for (int A = (!dir) ? (this->GetNode(N)->ArcDownId) : (this->GetNode(N)->ArcUpId);
               A; A = (!dir) ? (this->GetArc(A)->ArcDwId1) : (this->GetArc(A)->ArcDwId0))
            {
            int M = (!dir) ? (this->GetArc(A)->NodeId0) : (this->GetArc(A)->NodeId1);

            if (Atouch[A])
              continue;

            if (!Ntouch[M])
              {
              vtkReebGraphStackPush(M);
              }
            else
              {
              this->LoopNumber++;
              this->ArcLoopTable = (vtkIdType *) realloc(this->ArcLoopTable,
                  sizeof(vtkIdType)*this->LoopNumber);
              this->ArcLoopTable[this->LoopNumber-1]=A;
              }

            Atouch[A]=1;
            Ntouch[M]=1;
            }
          }
        }
      }
    }

  free(stack);
  free(Ntouch);
  free(Atouch);
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::AddMeshVertex(vtkIdType vertexId,
                                                      double scalar)
{
  static bool firstVertex = true;

  ScalarField[vertexId] = scalar;

  vtkIdType N0;
  ResizeMainNodeTable(1);

  //create a new node in the graph...
  N0 = this->MainNodeTable.FreeZone;
  this->MainNodeTable.FreeZone = this->GetNode(N0)->ArcDownId;
  ++(this->MainNodeTable.Number);
  memset(this->GetNode(N0),0,sizeof(vtkReebNode));

  vtkReebNode* node = this->GetNode(N0);
  node->VertexId=vertexId;
  node->Value = scalar;
  node->ArcDownId=0;
  node->ArcUpId=0;
  node->IsFinalized = false;

  if(firstVertex)
    {
    this->MinimumScalarValue = node->Value;
    this->MaximumScalarValue = node->Value;
    }
  else
    {
    if(node->Value > this->MaximumScalarValue)
      this->MaximumScalarValue = node->Value;
    if(node->Value < this->MinimumScalarValue)
      this->MinimumScalarValue = node->Value;
    }
  firstVertex = false;

  return N0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::FindDwLabel(vtkIdType nodeId,
                                                    vtkReebLabelTag label)
{
  for (vtkIdType arcId = this->GetNode(nodeId)->ArcDownId;
       arcId; arcId = this->GetArc(arcId)->ArcDwId1)
    {
    for (vtkIdType labelId = this->GetArc(arcId)->LabelId0;
         labelId; labelId = this->GetLabel(labelId)->HNext)
      {
      if (this->GetLabel(labelId)->label == label)
        return labelId;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::FindUpLabel(vtkIdType nodeId,
                                                    vtkReebLabelTag label)
{
  for (vtkIdType arcId = this->GetNode(nodeId)->ArcUpId;
       arcId; arcId = this->GetArc(arcId)->ArcDwId0)
    {
    for (vtkIdType labelId = this->GetArc(arcId)->LabelId0;
         labelId; labelId = this->GetLabel(labelId)->HNext)
      {
      if (this->GetLabel(labelId)->label==label)
        return labelId;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::ResizeMainArcTable(int newSize)
{
  int oldsize,i;
  if ((this->MainArcTable.Size-this->MainArcTable.Number)< newSize)
    {
    oldsize=this->MainArcTable.Size;
    if (!this->MainArcTable.Size) this->MainArcTable.Size= newSize;
    while ((this->MainArcTable.Size-this->MainArcTable.Number)< newSize)
      this->MainArcTable.Size<<=1;

    this->MainArcTable.Buffer =
      (vtkReebArc*)realloc(this->MainArcTable.Buffer,
        sizeof(vtkReebArc)*this->MainArcTable.Size);
    for (i=oldsize;i<this->MainArcTable.Size-1;i++)
      {
      this->GetArc(i)->LabelId0 = i+1;
      // clear arc
      this->GetArc(i)->LabelId1 = ((int)-2);
      }

    this->GetArc(i)->LabelId0 = this->MainArcTable.FreeZone;
    // clear arc
    this->GetArc(i)->LabelId1 = ((int)-2);
    this->MainArcTable.FreeZone=oldsize;
    }
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::ResizeMainLabelTable(int newSize)
{
  int oldsize,i;
  if ((this->MainLabelTable.Size-this->MainLabelTable.Number)< newSize)
    {
    oldsize=this->MainLabelTable.Size;
    if (!this->MainLabelTable.Size) this->MainLabelTable.Size = newSize;
    while ((this->MainLabelTable.Size-this->MainLabelTable.Number)< newSize)
      this->MainLabelTable.Size<<=1;

    this->MainLabelTable.Buffer = (vtkReebLabel*)realloc(this->MainLabelTable.Buffer, sizeof(vtkReebLabel)*this->MainLabelTable.Size);

    for (i=oldsize;i<this->MainLabelTable.Size-1;i++)
      {
      this->GetLabel(i)->ArcId = i+1;
      this->GetLabel(i)->HNext = ((int)-2);
      }

    this->GetLabel(i)->ArcId = this->MainLabelTable.FreeZone;
    this->GetLabel(i)->HNext = ((int)-2);
    this->MainLabelTable.FreeZone=oldsize;
    }
}


//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::AddPath(int nodeNumber,
                                                vtkIdType* nodeOffset,
                                                vtkReebLabelTag label)
{
  vtkIdType i, Lprev, Ret=0;

  this->ResizeMainArcTable(nodeNumber - 1);

  if (label)
    ResizeMainLabelTable(nodeNumber - 1);

  Lprev=0;
  for (i = 0; i < (nodeNumber - 1);i++)
    {
    vtkIdType N0 = nodeOffset[i ];
    vtkIdType N1= nodeOffset[i + 1];

    //create a new arc in the graph
    int A = this->MainArcTable.FreeZone;
    this->MainArcTable.FreeZone = this->GetArc(A)->LabelId0;
    ++(this->MainArcTable.Number);
    memset(this->GetArc(A),0,sizeof(vtkReebArc));
    vtkReebArc* arc = this->GetArc(A);

    int L=0;

    if (!Ret) Ret=A;

    if (label)
      {
      vtkReebLabel* temporaryLabel;

      //create a new label in the graph
      L = this->MainLabelTable.FreeZone;
      this->MainLabelTable.FreeZone = this->GetLabel(L)->ArcId;
      ++(this->MainLabelTable.Number);
      memset(this->GetLabel(L),0,sizeof(vtkReebLabel));
      temporaryLabel = this->GetLabel(L);

      temporaryLabel->ArcId = A;
      temporaryLabel->label=label;
      temporaryLabel->VPrev=Lprev;
      }

    arc->NodeId0=N0;
    arc->NodeId1=N1;
    arc->LabelId0=arc->LabelId1=L;

    vtkReebGraphAddUpArc(this,N0,A);
    vtkReebGraphAddDownArc(this,N1,A);

    if (label)
      {
      if (Lprev) this->GetLabel(Lprev)->VNext = L;
      Lprev=L;
      }
    }

  return Ret;
}


//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::Collapse(vtkIdType startingNode,
                                            vtkIdType endingNode,
                                            vtkReebLabelTag startingLabel,
                                            vtkReebLabelTag endingLabel)
{

  int L0,L0n,L1,L1n;
  int cont[3]={0,0,0},Case;

  if (startingNode == endingNode)
    return;

  vtkReebNode* nstart = this->GetNode(startingNode);
  vtkReebNode* nend = this->GetNode(endingNode);

  if (!vtkReebGraphIsSmaller(this,startingNode,endingNode,nstart,nend))
    {
    vtkReebGraphSwapVars(int,startingNode,endingNode);
    vtkReebGraphSwapVars(vtkReebNode* ,nstart,nend);
    }

  L0 = FindUpLabel(startingNode, startingLabel);
  L1 = FindUpLabel(startingNode, endingLabel);

  while (1)
    {
    int A0 = this->GetLabel(L0)->ArcId;
    vtkReebArc* a0 = this->GetArc(A0);
    int A1 = this->GetLabel(L1)->ArcId;
    vtkReebArc* a1 = this->GetArc(A1);

    vtkReebNode *down0 = this->GetNode(a0->NodeId0);
    vtkReebNode *up0 = this->GetNode(a0->NodeId1);
    vtkReebNode *up1 = this->GetNode(a1->NodeId1);

    /* it is the same arc, no semplification is done */
    if (A0==A1)
      {
      Case = 0;
      L0n = this->GetLabel(L0)->VNext;
      L1n = this->GetLabel(L1)->VNext;
      }
    /* there are two arcs connecting the same start-end node */
    else if (A0 != A1 && a0->NodeId1 == a1->NodeId1)
      {
      Case = 1;
      vtkReebGraphRemoveUpArc(this, a0->NodeId0, A1);
      vtkReebGraphRemoveDownArc(this, a0->NodeId1, A1);

      for (int Lcur = this->GetArc(A1)->LabelId0;
           Lcur; Lcur = this->GetLabel(Lcur)->HNext)
        {
        this->GetLabel(Lcur)->ArcId = A0;
        }

      this->GetLabel(this->GetArc(A1)->LabelId0)->HPrev = this->GetArc(A0)->LabelId1;
      this->GetLabel(this->GetArc(A0)->LabelId1)->HNext = this->GetArc(A1)->LabelId0;
      this->GetArc(A0)->LabelId1 = this->GetArc(A1)->LabelId1;

      this->GetArc(A1)->LabelId0=0;
      this->GetArc(A1)->LabelId1=0;

      //delete the arc from the graph...
      this->GetArc(A1)->LabelId1 = ((int)-2);
      this->GetArc(A1)->LabelId0 = this->MainArcTable.FreeZone;
      this->MainArcTable.FreeZone=(A1);
      --(this->MainArcTable.Number);

      L0n = this->GetLabel(L0)->VNext;
      L1n = this->GetLabel(L1)->VNext;
      }
    else
      {
      if(historyOn)
        {
        vtkReebCancellation c;
        int downVertex, middleVertex, upVertex;
        downVertex = down0->VertexId;
        middleVertex = up0->VertexId;
        upVertex = up1->VertexId;
        c.removedArcs.push_back(std::pair<int, int>(downVertex, upVertex));
        c.insertedArcs.push_back(std::pair<int, int>(downVertex, middleVertex));
        c.insertedArcs.push_back(std::pair<int, int>(middleVertex, upVertex));
        this->cancellationHistory.push_back(c);
        }
      // a more complicate situation, collapse reaching the less ending point of
      // the arcs.
      Case=2;
        {
        vtkReebNode* a0n1 = this->GetNode(a0->NodeId1);
        vtkReebNode* a1n1 = this->GetNode(a1->NodeId1);
        if (!vtkReebGraphIsSmaller(this,a0->NodeId1,a1->NodeId1,a0n1,a1n1))
          {
          vtkReebGraphSwapVars(int,A0,A1);
          vtkReebGraphSwapVars(int,L0,L1);
          vtkReebGraphSwapVars(vtkReebArc* ,a0,a1);
          }
        }

      vtkReebGraphRemoveUpArc(this,a0->NodeId0,A1);
      a1->NodeId0=a0->NodeId1;
      vtkReebGraphAddUpArc(this,a0->NodeId1,A1);

      //"replicate" labels from A1 to A0
      for (int Lcur = this->GetArc(A1)->LabelId0;
        Lcur;Lcur = this->GetLabel(Lcur)->HNext)
        {
        int Lnew;
        ResizeMainLabelTable(1);

        //create a new label in the graph
        Lnew = this->MainLabelTable.FreeZone;
        this->MainLabelTable.FreeZone = this->GetLabel(Lnew)->ArcId;
        ++(this->MainLabelTable.Number);
        memset(this->GetLabel(Lnew),0,sizeof(vtkReebLabel));

        vtkReebLabel* lnew = this->GetLabel(Lnew);
        vtkReebLabel* lcur = this->GetLabel(Lcur);
        lnew->ArcId = A0;
        lnew->VPrev = lcur->VPrev;

        if (lcur->VPrev)
          this->GetLabel(lcur->VPrev)->VNext=Lnew;

        lcur->VPrev = Lnew;
        lnew->VNext = Lcur;
        lnew->label = lcur->label;

        lnew->HNext = 0;
        lnew->HPrev = this->GetArc(A0)->LabelId1;
        this->GetLabel(this->GetArc(A0)->LabelId1)->HNext = Lnew;

        this->GetArc(A0)->LabelId1=Lnew;
        }

      L0n = this->GetLabel(L0)->VNext;
      L1n=L1;
      }

    ++cont[Case];

    int N0=a0->NodeId0;
    vtkReebNode* n0 = this->GetNode(N0);

    if (n0->IsFinalized && vtkReebGraphIsRegular(this,n0))
      {
      if(historyOn)
        {
        vtkReebArc *up = this->GetArc(n0->ArcUpId),
          *down = this->GetArc(n0->ArcDownId);

        vtkReebCancellation c;
        int v0, v1, v2, v3;

        v0 = this->GetNode(up->NodeId0)->VertexId;
        v1 = this->GetNode(up->NodeId1)->VertexId;
        v2 = this->GetNode(down->NodeId0)->VertexId;
        v3 = this->GetNode(down->NodeId1)->VertexId;

        c.removedArcs.push_back(std::pair<int, int>(v0, v1));
        c.removedArcs.push_back(std::pair<int, int>(v2, v3));
        c.insertedArcs.push_back(std::pair<int, int>(v2, v1));
        this->cancellationHistory.push_back(c);
        }
      this->CollapseVertex(N0, n0);
      }

    /* end condition */
    if (a0->NodeId1 == endingNode)
      {
      vtkReebNode* nendNode = this->GetNode(endingNode);

      if (nendNode->IsFinalized && vtkReebGraphIsRegular(this, nendNode))
        {
        if(historyOn)
          {
          vtkReebArc *up = this->GetArc(this->GetNode(endingNode)->ArcUpId),
            *down = this->GetArc(this->GetNode(endingNode)->ArcDownId);

          vtkReebCancellation c;

          int v0, v1, v2, v3;
          v0 = this->GetNode(up->NodeId0)->VertexId;
          v1 = this->GetNode(up->NodeId1)->VertexId;
          v2 = this->GetNode(down->NodeId0)->VertexId;
          v3 = this->GetNode(down->NodeId1)->VertexId;

          c.removedArcs.push_back(std::pair<int, int>(v0, v1));
          c.removedArcs.push_back(std::pair<int, int>(v2, v3));
          c.insertedArcs.push_back(std::pair<int, int>(v2, v1));
          this->cancellationHistory.push_back(c);
          }
        this->CollapseVertex(endingNode, nendNode);
        }

      return;
      }

    L0 = L0n;
    L1 = L1n;
    }
}

void vtkReebGraph::Implementation::SimplifyLabels(const vtkIdType nodeId,
                                                  vtkReebLabelTag onlyLabel,
                                                  bool goDown, bool goUp)
{
  static int nactivation=0;
  ++nactivation;

  int A,L,Lnext;
  vtkReebLabel *l;
  vtkReebNode *n = this->GetNode(nodeId);

  //I remove all Labels (paths) which start from me
  if (goDown)
    {
    int Anext;
    for (A=n->ArcDownId;A;A=Anext)
      {
      Anext = this->GetArc(A)->ArcDwId1;
      for (L = this->GetArc(A)->LabelId0;L;L=Lnext)
        {
        Lnext = this->GetLabel(L)->HNext;

        if (!(l = this->GetLabel(L))->VNext)  //...starts from me!
          {
          if (!onlyLabel || onlyLabel == this->GetLabel(L)->label)
            {
            int Lprev;
            for (int Lcur=L;Lcur;Lcur=Lprev)
              {
              vtkReebLabel* lcur = this->GetLabel(Lcur);
              Lprev=lcur->VPrev;
              int CurA=lcur->ArcId;
              if (lcur->HPrev)
                this->GetLabel(lcur->HPrev)->HNext = lcur->HNext;
              else this->GetArc(CurA)->LabelId0 = lcur->HNext;
              if (lcur->HNext)
                this->GetLabel(lcur->HNext)->HPrev = lcur->HPrev;
              else this->GetArc(CurA)->LabelId1 = lcur->HPrev;

              //delete the label
              this->GetLabel(Lcur)->HNext = ((int)-2);
              this->GetLabel(Lcur)->ArcId = this->MainLabelTable.FreeZone;
              this->MainLabelTable.FreeZone=(Lcur);
              --(this->MainLabelTable.Number);
              }
            }
          }
        }
      }
    }

  // Remove all Labels (paths) which start from here

  if (goUp && !(this->GetNode(nodeId)->ArcUpId == ((int)-2)))
    {
    int Anext;
    for (A=n->ArcUpId;A;A=Anext)
      {
      Anext = this->GetArc(A)->ArcDwId0;
      for (L=this->GetArc(A)->LabelId0;L;L=Lnext)
        {
        Lnext = this->GetLabel(L)->HNext;

        if (!(l = this->GetLabel(L))->VPrev)  //...starts from me!
          {
          if (!onlyLabel || onlyLabel == this->GetLabel(L)->label)
            {
            int myLnext;
            for (int Lcur = L; Lcur; Lcur = myLnext)
              {
              vtkReebLabel* lcur = this->GetLabel(Lcur);
              myLnext = lcur->VNext;
              int CurA = lcur->ArcId;
              vtkReebArc* cura = this->GetArc(CurA);
              if (lcur->HPrev)
                {
                this->GetLabel(lcur->HPrev)->HNext = lcur->HNext;
                }
              else
                {
                cura->LabelId0=lcur->HNext;
                }
              if (lcur->HNext)
                {
                this->GetLabel(lcur->HNext)->HPrev = lcur->HPrev;
                }
              else
                {
                cura->LabelId1=lcur->HPrev;
                }

              //delete the label...
              this->GetLabel(Lcur)->HNext = ((int)-2);
              this->GetLabel(Lcur)->ArcId = this->MainLabelTable.FreeZone;
              this->MainLabelTable.FreeZone=(Lcur);
              --(this->MainLabelTable.Number);
              }
            }
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkReebGraph::Implementation::EndVertex(const vtkIdType N)
{
  vtkReebNode *n = this->GetNode(N);

  n->IsFinalized = true;

  if (!(this->GetNode(N)->ArcUpId == ((int)-2)))
    {
    this->SimplifyLabels(N);

    if (!(this->GetNode(N)->ArcUpId == ((int)-2)))
      {
      //special case for regular point. A node is regular if it has one
      //arc down and one arc up. In this case it can disappear

      if (vtkReebGraphIsRegular(this,n))
        {
        this->CollapseVertex(N, n);
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::AddMeshTetrahedron(vtkIdType vertex0Id,
                                                     double f0,
                                                     vtkIdType vertex1Id,
                                                     double f1,
                                                     vtkIdType vertex2Id,
                                                     double f2,
                                                     vtkIdType vertex3Id,
                                                     double f3)
{
  vtkIdType vertex0, vertex1, vertex2, vertex3;

  vertex0 = this->VertexStream[vertex0Id];
  vertex1 = this->VertexStream[vertex1Id];
  vertex2 = this->VertexStream[vertex2Id];
  vertex3 = this->VertexStream[vertex3Id];

  int N0 = this->VertexMap[vertex0];
  int N1 = this->VertexMap[vertex1];
  int N2 = this->VertexMap[vertex2];
  int N3 = this->VertexMap[vertex3];

  // Consistency less check
  if (f3 < f2 || (f3==f2 && vertex3 < vertex2))
    {
    vtkReebGraphSwapVars(int,vertex2,vertex3);
    vtkReebGraphSwapVars(int,N2,N3);
    vtkReebGraphSwapVars(double,f2,f3);
    }
  if (f2 < f1 || (f2==f1 && vertex2 < vertex1))
    {
    vtkReebGraphSwapVars(int,vertex1,vertex2);
    vtkReebGraphSwapVars(int,N1,N2);
    vtkReebGraphSwapVars(double,f1,f2);
    }
  if (f1 < f0 || (f1==f0 && vertex1 < vertex0))
    {
    vtkReebGraphSwapVars(int,vertex0,vertex1);
    vtkReebGraphSwapVars(int,N0,N1);
    vtkReebGraphSwapVars(double,f0,f1);
    }
  if (f3 < f2 || (f3==f2 && vertex3 < vertex2))
    {
    vtkReebGraphSwapVars(int,vertex2,vertex3);
    vtkReebGraphSwapVars(int,N2,N3);
    vtkReebGraphSwapVars(double,f2,f3);
    }
  if (f2 < f1 || (f2==f1 && vertex2 < vertex1))
    {
    vtkReebGraphSwapVars(int,vertex1,vertex2);
    vtkReebGraphSwapVars(int,N1,N2);
    vtkReebGraphSwapVars(double,f1,f2);
    }
  if (f3 < f2 || (f3==f2 && vertex3 < vertex2))
    {
    vtkReebGraphSwapVars(int,vertex2,vertex3);
    vtkReebGraphSwapVars(int,N2,N3);
    vtkReebGraphSwapVars(double,f2,f3);
    }

  vtkIdType t0[]={vertex0, vertex1, vertex2},
            t1[]={vertex0, vertex1, vertex3},
            t2[]={vertex0, vertex2, vertex3},
            t3[]={vertex1, vertex2, vertex3};
  vtkIdType *cellIds[4];
  cellIds[0] = t0;
  cellIds[1] = t1;
  cellIds[2] = t2;
  cellIds[3] = t3;

  for(int i=0; i < 3; i++)
    {
    int n0 = this->VertexMap[cellIds[i][0]],
        n1 = this->VertexMap[cellIds[i][1]],
        n2 = this->VertexMap[cellIds[i][2]];

    vtkReebLabelTag Label01 =
      ((vtkReebLabelTag) cellIds[i][0])
      | (((vtkReebLabelTag) cellIds[i][1])<<32);
    vtkReebLabelTag Label12 =
      ((vtkReebLabelTag) cellIds[i][1])
      | (((vtkReebLabelTag) cellIds[i][2])<<32);
    vtkReebLabelTag Label02 =
      ((vtkReebLabelTag) cellIds[i][0])
      | (((vtkReebLabelTag) cellIds[i][2])<<32);

    if (!this->FindUpLabel(n0,Label01))
      {
      vtkIdType N01[] = {n0, n1};
      this->AddPath(2, N01, Label01);
      }
    if (!this->FindUpLabel(n1,Label12))
      {
      vtkIdType N12[] = {n1,n2};
      this->AddPath(2, N12, Label12);
      }
    if (!this->FindUpLabel(n0, Label02))
      {
      vtkIdType N02[] = {n0,n2};
      this->AddPath(2, N02, Label02);
      }

    this->Collapse(n0,n1,Label01,Label02);
    this->Collapse(n1,n2,Label12,Label02);
    }

  if (!(--(this->TriangleVertexMap[vertex0])))
    this->EndVertex(N0);
  if (!(--(this->TriangleVertexMap[vertex1])))
    this->EndVertex(N1);
  if (!(--(this->TriangleVertexMap[vertex2])))
    this->EndVertex(N2);
  if (!(--(this->TriangleVertexMap[vertex3])))
    this->EndVertex(N3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::AddMeshTriangle(vtkIdType vertex0Id, double f0,
                                                  vtkIdType vertex1Id, double f1,
                                                  vtkIdType vertex2Id, double f2)
{

  int vertex0 = this->VertexStream[vertex0Id],
      vertex1 = this->VertexStream[vertex1Id],
      vertex2 = this->VertexStream[vertex2Id];

  int N0 = this->VertexMap[vertex0];
  int N1 = this->VertexMap[vertex1];
  int N2 = this->VertexMap[vertex2];

  // Consistency less check
  if (f2 < f1 || (f2==f1 && vertex2 < vertex1))
    {
    vtkReebGraphSwapVars(int,vertex1,vertex2);
    vtkReebGraphSwapVars(int,N1,N2);
    vtkReebGraphSwapVars(double,f1,f2);
    }
  if (f1 < f0 || (f1==f0 && vertex1 < vertex0))
    {
    vtkReebGraphSwapVars(int,vertex0,vertex1);
    vtkReebGraphSwapVars(int,N0,N1);
    vtkReebGraphSwapVars(double,f0,f1);
    }
  if (f2 < f1 || (f2==f1 && vertex2 < vertex1))
    {
    vtkReebGraphSwapVars(int,vertex1,vertex2);
    vtkReebGraphSwapVars(int,N1,N2);
    vtkReebGraphSwapVars(double,f1,f2);
    }

  vtkReebLabelTag Label01 =
    ((vtkReebLabelTag)vertex0) | (((vtkReebLabelTag)vertex1)<<32);
  vtkReebLabelTag Label12 =
    ((vtkReebLabelTag)vertex1) | (((vtkReebLabelTag)vertex2)<<32);
  vtkReebLabelTag Label02 =
    ((vtkReebLabelTag)vertex0) | (((vtkReebLabelTag)vertex2)<<32);

  if (!this->FindUpLabel(N0,Label01))
    {
    vtkIdType N01[] = {N0, N1};
    this->AddPath(2, N01, Label01);
    }
  if (!this->FindUpLabel(N1,Label12))
    {
    vtkIdType N12[] = {N1,N2};
    this->AddPath(2, N12, Label12);
    }
  if (!this->FindUpLabel(N0, Label02))
    {
    vtkIdType N02[] = {N0,N2};
    this->AddPath(2, N02, Label02);
    }

  this->Collapse(N0,N1,Label01,Label02);
  this->Collapse(N1,N2,Label12,Label02);

  if (!(--(this->TriangleVertexMap[vertex0])))
    this->EndVertex(N0);
  if (!(--(this->TriangleVertexMap[vertex1])))
    this->EndVertex(N1);
  if (!(--(this->TriangleVertexMap[vertex2])))
    this->EndVertex(N2);

  return 1;
}

//----------------------------------------------------------------------------
int vtkReebGraph::StreamTetrahedron( vtkIdType vertex0Id, double scalar0,
                                     vtkIdType vertex1Id, double scalar1,
                                     vtkIdType vertex2Id, double scalar2,
                                     vtkIdType vertex3Id, double scalar3)
{
  return this->Storage->StreamTetrahedron(vertex0Id, scalar0,
                                          vertex1Id, scalar1,
                                          vertex2Id, scalar2,
                                          vertex3Id, scalar3);
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::StreamTetrahedron( vtkIdType vertex0Id,
                                                    double scalar0,
                                                    vtkIdType vertex1Id,
                                                    double scalar1,
                                                    vtkIdType vertex2Id,
                                                    double scalar2,
                                                    vtkIdType vertex3Id,
                                                    double scalar3)
{
  if(!this->VertexMapAllocatedSize)
    {
    // first allocate an arbitrary size
    this->VertexMapAllocatedSize = vtkReebGraphInitialStreamSize;
    this->VertexMap = (vtkIdType *) malloc(
      sizeof(vtkIdType)*this->VertexMapAllocatedSize);
    memset(this->VertexMap, 0, sizeof(vtkIdType)*this->VertexMapAllocatedSize);
    this->VertexStream.clear();
    }
  else if(this->VertexMapSize >= this->VertexMapAllocatedSize - 4)
    {
    int oldSize = this->VertexMapAllocatedSize;
    this->VertexMapAllocatedSize <<= 1;
    this->VertexMap = (vtkIdType *) realloc(this->VertexMap,
      sizeof(vtkIdType)*this->VertexMapAllocatedSize);
    for(int i = oldSize; i < this->VertexMapAllocatedSize - 1; i++)
      this->VertexMap[i] = 0;
    }

  // same thing with the triangle map
  if(!this->TriangleVertexMapAllocatedSize)
    {
    // first allocate an arbitrary size
    this->TriangleVertexMapAllocatedSize = vtkReebGraphInitialStreamSize;
    this->TriangleVertexMap = (int *) malloc(
      sizeof(int)*this->TriangleVertexMapAllocatedSize);
    memset(this->TriangleVertexMap, 0,
      sizeof(int)*this->TriangleVertexMapAllocatedSize);
    }
  else if(this->TriangleVertexMapSize >=
    this->TriangleVertexMapAllocatedSize - 4)
    {
    int oldSize = this->TriangleVertexMapAllocatedSize;
    this->TriangleVertexMapAllocatedSize <<= 1;
    this->TriangleVertexMap = (int *) realloc(this->TriangleVertexMap,
                                              sizeof(int)*this->TriangleVertexMapAllocatedSize);
    for(int i = oldSize; i < this->TriangleVertexMapAllocatedSize - 1; i++)
      this->TriangleVertexMap[i] = 0;
    }

  // Add the vertices to the stream
  std::map<int, int>::iterator sIter;

  // vertex0
  sIter = this->VertexStream.find(vertex0Id);
  if(sIter == this->VertexStream.end())
    {
    // this vertex hasn't been streamed yet, let's add it
    this->VertexStream[vertex0Id] = this->VertexMapSize;
    this->VertexMap[this->VertexMapSize]
      = this->AddMeshVertex(vertex0Id, scalar0);
    this->VertexMapSize++;
    this->TriangleVertexMapSize++;
    }

  // vertex1
  sIter = this->VertexStream.find(vertex1Id);
  if(sIter == this->VertexStream.end())
    {
    // this vertex hasn't been streamed yet, let's add it
    this->VertexStream[vertex1Id] = this->VertexMapSize;
    this->VertexMap[this->VertexMapSize]
      = this->AddMeshVertex(vertex1Id, scalar1);
    this->VertexMapSize++;
    this->TriangleVertexMapSize++;
    }

  // vertex2
  sIter = this->VertexStream.find(vertex2Id);
  if(sIter == this->VertexStream.end())
    {
    // this vertex hasn't been streamed yet, let's add it
    this->VertexStream[vertex2Id] = this->VertexMapSize;
    this->VertexMap[this->VertexMapSize]
      = this->AddMeshVertex(vertex2Id, scalar2);
    this->VertexMapSize++;
    this->TriangleVertexMapSize++;
    }

  // vertex3
  sIter = this->VertexStream.find(vertex3Id);
  if(sIter == this->VertexStream.end())
    {
    // this vertex hasn't been streamed yet, let's add it
    this->VertexStream[vertex3Id] = this->VertexMapSize;
    this->VertexMap[this->VertexMapSize]
      = this->AddMeshVertex(vertex3Id, scalar3);
    this->VertexMapSize++;
    this->TriangleVertexMapSize++;
    }

  this->AddMeshTetrahedron(vertex0Id, scalar0, vertex1Id, scalar1,
    vertex2Id, scalar2, vertex3Id, scalar3);

  return 0;
}

//----------------------------------------------------------------------------
int vtkReebGraph::StreamTriangle(	vtkIdType vertex0Id, double scalar0,
                                  vtkIdType vertex1Id, double scalar1,
                                  vtkIdType vertex2Id, double scalar2)
{
  return this->Storage->StreamTriangle(vertex0Id, scalar0,
                                       vertex1Id, scalar1,
                                       vertex2Id, scalar2);
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::StreamTriangle(vtkIdType vertex0Id,
                                                 double scalar0,
                                                 vtkIdType vertex1Id,
                                                 double scalar1,
                                                 vtkIdType vertex2Id,
                                                 double scalar2)
{
  if(!this->VertexMapAllocatedSize)
    {
    // first allocate an arbitrary size
    this->VertexMapAllocatedSize = vtkReebGraphInitialStreamSize;
    this->VertexMap = (vtkIdType *) malloc(
      sizeof(vtkIdType)*this->VertexMapAllocatedSize);
    memset(this->VertexMap, 0, sizeof(vtkIdType)*this->VertexMapAllocatedSize);
    }
  else if(this->VertexMapSize >= this->VertexMapAllocatedSize - 3)
    {
    int oldSize = this->VertexMapAllocatedSize;
    this->VertexMapAllocatedSize <<= 1;
    this->VertexMap = (vtkIdType *) realloc(this->VertexMap,
      sizeof(vtkIdType)*this->VertexMapAllocatedSize);
    for(int i = oldSize; i < this->VertexMapAllocatedSize - 1; i++)
      this->VertexMap[i] = 0;
    }

  // same thing with the triangle map
  if(!this->TriangleVertexMapAllocatedSize)
    {
    // first allocate an arbitrary size
    this->TriangleVertexMapAllocatedSize = vtkReebGraphInitialStreamSize;
    this->TriangleVertexMap = (int *) malloc(
      sizeof(int)*this->TriangleVertexMapAllocatedSize);
    memset(this->TriangleVertexMap, 0,
      sizeof(int)*this->TriangleVertexMapAllocatedSize);
    }
  else if(this->TriangleVertexMapSize >=
    this->TriangleVertexMapAllocatedSize - 3)
    {
      int oldSize = this->TriangleVertexMapAllocatedSize;
      this->TriangleVertexMapAllocatedSize <<= 1;
      this->TriangleVertexMap = (int *) realloc(this->TriangleVertexMap,
        sizeof(int)*this->TriangleVertexMapAllocatedSize);
      for(int i = oldSize; i < this->TriangleVertexMapAllocatedSize - 1; i++)
        this->TriangleVertexMap[i] = 0;
    }

  // Add the vertices to the stream
  std::map<int, int>::iterator sIter;

  // vertex0
  sIter = this->VertexStream.find(vertex0Id);
  if(sIter == this->VertexStream.end())
    {
    // this vertex hasn't been streamed yet, let's add it
    this->VertexStream[vertex0Id] = this->VertexMapSize;
    this->VertexMap[this->VertexMapSize]
      = this->AddMeshVertex(vertex0Id, scalar0);
    this->VertexMapSize++;
    this->TriangleVertexMapSize++;
    }

  // vertex1
  sIter = this->VertexStream.find(vertex1Id);
  if(sIter == this->VertexStream.end())
    {
    // this vertex hasn't been streamed yet, let's add it
    this->VertexStream[vertex1Id] = this->VertexMapSize;
    this->VertexMap[this->VertexMapSize]
      = this->AddMeshVertex(vertex1Id, scalar1);
    this->VertexMapSize++;
    this->TriangleVertexMapSize++;
    }

  // vertex2
  sIter = this->VertexStream.find(vertex2Id);
  if(sIter == this->VertexStream.end())
    {
    // this vertex hasn't been streamed yet, let's add it
    this->VertexStream[vertex2Id] = this->VertexMapSize;
    this->VertexMap[this->VertexMapSize]
      = this->AddMeshVertex(vertex2Id, scalar2);
    this->VertexMapSize++;
    this->TriangleVertexMapSize++;
    }

  this->AddMeshTriangle(vertex0Id, scalar0, vertex1Id, scalar1,
    vertex2Id, scalar2);

  return 0;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkPolyData *mesh, vtkDataArray *scalarField)
{
  for(vtkIdType i = 0; i < mesh->GetNumberOfCells(); i++)
    {
    vtkCell *triangle = mesh->GetCell(i);
    vtkIdList *trianglePointList = triangle->GetPointIds();
    if(trianglePointList->GetNumberOfIds() != 3)
      return vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH;
    StreamTriangle(trianglePointList->GetId(0),
      scalarField->GetComponent(trianglePointList->GetId(0),0),
      trianglePointList->GetId(1),
      scalarField->GetComponent(trianglePointList->GetId(1),0),
      trianglePointList->GetId(2),
      scalarField->GetComponent(trianglePointList->GetId(2),0));
    }

  this->Storage->inputMesh = mesh;
  this->Storage->inputScalarField = scalarField;

  this->CloseStream();

  return 0;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkUnstructuredGrid *mesh, vtkDataArray *scalarField)
{
  for(vtkIdType i = 0; i < mesh->GetNumberOfCells(); i++)
    {
    vtkCell *tet = mesh->GetCell(i);
    vtkIdList *tetPointList = tet->GetPointIds();
    if(tetPointList->GetNumberOfIds() != 4)
      return vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH;
    StreamTetrahedron(tetPointList->GetId(0),
      scalarField->GetComponent(tetPointList->GetId(0),0),
      tetPointList->GetId(1),
      scalarField->GetComponent(tetPointList->GetId(1),0),
      tetPointList->GetId(2),
      scalarField->GetComponent(tetPointList->GetId(2),0),
      tetPointList->GetId(3),
      scalarField->GetComponent(tetPointList->GetId(3),0));
    }

  this->Storage->inputMesh = mesh;
  this->Storage->inputScalarField = scalarField;

  this->CloseStream();

  return 0;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::GetNumberOfArcs()
{
  if(!this->ArcNumber)
    for(vtkIdType arcId = 1; arcId < this->MainArcTable.Size; arcId++)
      {
      // check if arc is cleared
      if(!(this->GetArc(arcId)->LabelId1 == ((int)-2)))
        this->ArcNumber++;
      }

  return this->ArcNumber;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::GetNumberOfConnectedComponents()
{
  if(!this->ArcLoopTable) this->FindLoops();
  return this->ConnectedComponentNumber;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::GetNumberOfNodes()
{
  if(!this->NodeNumber)
    for(vtkIdType nodeId = 1; nodeId < this->MainNodeTable.Size; nodeId++)
      {
      // check if node is cleared
      if(!(this->GetNode(nodeId)->ArcUpId == ((int)-2)))
        this->NodeNumber++;
      }

  return this->NodeNumber;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::GetNextNodeId()
{
  for(vtkIdType nodeId = this->currentNodeId + 1;
    nodeId < this->MainNodeTable.Size; nodeId++)
    {
    // check if node is cleared
    if(!(this->GetNode(nodeId)->ArcUpId == ((int)-2)))
      {
      this->currentNodeId = nodeId;
      return this->currentNodeId;
      }
    }

  return this->currentNodeId;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::GetPreviousNodeId()
{
  if(!this->currentNodeId)
    {
    return this->GetNextNodeId();
    }

  for(vtkIdType nodeId = this->currentNodeId - 1; nodeId > 0; nodeId--)
    {
    // check if node is cleared
    if(!(this->GetNode(nodeId)->ArcUpId == ((int)-2)))
      {
      this->currentNodeId = nodeId;
      return this->currentNodeId;
      }
    }

  return this->currentNodeId;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::GetNextArcId()
{
  for(vtkIdType arcId = this->currentArcId + 1; arcId < this->MainArcTable.Size; arcId++)
    {
    // check if arc is cleared
    if(!(this->GetArc(arcId)->LabelId1 == ((int)-2)))
      {
      this->currentArcId = arcId;
      return this->currentArcId;
      }
    }

  return this->currentArcId;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::GetPreviousArcId()
{
  if(!this->currentArcId)
    {
    return this->GetNextArcId();
    }

  for(vtkIdType arcId = this->currentArcId - 1; arcId > 0; arcId--)
    {
    // check if arc is cleared
    if(!(this->GetArc(arcId)->LabelId1 == ((int)-2)))
      {
      this->currentArcId = arcId;
      return this->currentArcId;
      }
    }

  return this->currentArcId;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::GetArcDownNodeId(vtkIdType arcId)
{
  return (this->GetArc(arcId))->NodeId0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::GetArcUpNodeId(vtkIdType arcId)
{
  return (this->GetArc(arcId))->NodeId1;
}

//----------------------------------------------------------------------------
double vtkReebGraph::Implementation::GetNodeScalarValue(vtkIdType nodeId)
{
  return (this->GetNode(nodeId))->Value;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::Implementation::GetNodeVertexId(vtkIdType nodeId)
{
  return (this->GetNode(nodeId))->VertexId;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkPolyData *mesh, vtkIdType scalarFieldId)
{
  vtkPointData *pointData = mesh->GetPointData();
  vtkDataArray *scalarField = pointData->GetArray(scalarFieldId);

  if(!scalarField) return vtkReebGraph::ERR_NO_SUCH_FIELD;

  return this->Build(mesh, scalarField);
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkUnstructuredGrid *mesh, vtkIdType scalarFieldId)
{
  vtkPointData *pointData = mesh->GetPointData();
  vtkDataArray *scalarField = pointData->GetArray(scalarFieldId);

  if(!scalarField) return vtkReebGraph::ERR_NO_SUCH_FIELD;

  return this->Build(mesh, scalarField);
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkPolyData *mesh, const char* scalarFieldName)
{
  int scalarFieldId = 0;

  vtkPointData *pointData = mesh->GetPointData();
  vtkDataArray *scalarField =
    pointData->GetArray(scalarFieldName, scalarFieldId);

  if(!scalarField) return vtkReebGraph::ERR_NO_SUCH_FIELD;

  return this->Build(mesh, scalarField);
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkUnstructuredGrid *mesh, const char* scalarFieldName)
{
  int scalarFieldId = 0;

  vtkPointData *pointData = mesh->GetPointData();
  vtkDataArray *scalarField =
    pointData->GetArray(scalarFieldName, scalarFieldId);

  if(!scalarField) return vtkReebGraph::ERR_NO_SUCH_FIELD;

  return this->Build(mesh, scalarField);
}

//----------------------------------------------------------------------------
int vtkReebGraph::Implementation::GetNumberOfLoops()
{
  if(!this->ArcLoopTable) this->FindLoops();
  return this->LoopNumber - this->RemovedLoopNumber;
}

//----------------------------------------------------------------------------
inline vtkIdType vtkReebGraph::Implementation::AddArc(vtkIdType nodeId0,
                                                      vtkIdType nodeId1)
{
  if (!vtkReebGraphIsSmaller(this, nodeId0, nodeId1,
                             this->GetNode(nodeId0),
                             this->GetNode(nodeId1)))
    vtkReebGraphSwapVars(vtkIdType, nodeId0, nodeId1);
  vtkIdType nodevtkReebArcble[] = { nodeId0, nodeId1};
  return AddPath(2, nodevtkReebArcble, 0);
}
