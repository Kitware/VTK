/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecimatePro.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkDecimatePro.h"
#include "vtkMath.h"
#include "vtkTriangle.h"
#include "vtkPlane.h"
#include "vtkLine.h"

#define VTK_TOLERANCE 1.0e-05
#define VTK_MAX_TRIS_PER_VERTEX VTK_CELL_SIZE
#define VTK_RECYCLE_VERTEX VTK_LARGE_FLOAT

#define VTK_SIMPLE_VERTEX 1
#define VTK_BOUNDARY_VERTEX 2
#define VTK_INTERIOR_EDGE_VERTEX 3
#define VTK_CORNER_VERTEX 4
#define VTK_CRACK_TIP_VERTEX 5
#define VTK_EDGE_END_VERTEX 6
#define VTK_NON_MANIFOLD_VERTEX 7
#define VTK_DEGENERATE_VERTEX 8
#define VTK_HIGH_DEGREE_VERTEX 9

#define VTK_STATE_UNSPLIT 0
#define VTK_STATE_SPLIT 1
#define VTK_STATE_SPLIT_ALL 2

// Static variables used by object
static vtkPolyData *Mesh; //operate on this data structure
static float Pt[3], Normal[3]; //least squares plane point & normal
static float LoopArea;   //the total area of all triangles in a loop
static float CosAngle;   //Cosine of dihedral angle
static float Tolerance;  //Intersection tolerance
static float X[3];       //coordinates of current point
static vtkVertexArray *V;//cycle of vertices around point
static vtkTriArray *T;   //cycle of triangles around point
static int NumCollapses; //Number of times edge collapses occur
static int NumMerges;    //Number of times vertex merges occur
static int Split;        //Controls whether and when vertex splitting occurs
static int VertexDegree; //Maximum number of triangles that can use a vertex
static int NumberOfRemainingTris; //Number of triangles left in the mesh
static float TheSplitAngle; //Split angle
static int SplitState;   //State of the splitting process
static float Error;      //Maximum allowable surface error


// Helper functions
static float ComputeSimpleError(float x[3], float normal[3], float point[3]);
static float ComputeEdgeError(float x[3], float x1[3], float x2[3]);
static float ComputeSingleTriangleError(float x[3], float x1[3], float x2[3]);


// Description:
// Create object with specified reduction of 90% and feature angle of
// 15 degrees. Edge splitting is on, defer splitting is on, and the
// split angle is 75 degrees. Topology preservation is off, delete
// boundary vertices is on, and the maximum error is set to
// VTK_LARGE_FLOAT. The inflection point ratio is 10 and the vertex
// degree is 25. Error accumulation is turned off.
vtkDecimatePro::vtkDecimatePro()
{
  this->TargetReduction = 0.90;
  this->FeatureAngle = 15.0;
  this->PreserveTopology = 0;
  this->MaximumError = VTK_LARGE_FLOAT;
  this->AccumulateError = 0;
  this->SplitAngle = 75.0;
  this->Splitting = 1;
  this->PreSplitMesh = 0;
  this->Degree = 25;
  this->BoundaryVertexDeletion = 1;
  this->InflectionPointRatio = 10.0;

  this->Queue = NULL;
  this->VertexError = NULL;
}

vtkDecimatePro::~vtkDecimatePro()
{
  if ( this->Queue ) delete this->Queue;
  if ( this->VertexError ) delete this->VertexError;
}

//
//  Reduce triangles in mesh by specified reduction factor.
//
void vtkDecimatePro::Execute()
{
  int i, ptId, numPts, numTris, collapseId;
  vtkPoints *inPts;
  vtkFloatPoints *newPts;
  vtkCellArray *inPolys;
  vtkCellArray *newPolys;
  float error, previousError=0.0, reduction;
  vtkPolyData *input=(vtkPolyData *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  int type, totalEliminated, numPops;
  int numRecycles, npts, *pts;
  unsigned short int ncells;
  int *cells, pt1, pt2, cellId, fedges[2];
  vtkIdList CollapseTris(100,100);
  float max, *bounds;

  // do it this way because some compilers can't handle construction of
  // static objects in file scope.
  static vtkVertexArray VertexArray(VTK_MAX_TRIS_PER_VERTEX+1);
  static vtkTriArray TriangleArray(VTK_MAX_TRIS_PER_VERTEX+1);
  V = &VertexArray;
  T = &TriangleArray;

  vtkDebugMacro(<<"Executing progressive decimation...");

  // Check input
  NumberOfRemainingTris = numTris = input->GetNumberOfPolys();
  if ( (numPts=input->GetNumberOfPoints()) < 1 || numTris < 1 )
    {
    vtkErrorMacro(<<"No data to decimate!");
    return;
    }

  // Initialize
  bounds = input->GetBounds();
  for (max=0.0, i=0; i<3; i++)
    max = ((bounds[2*i+1]-bounds[2*i]) > max ? 
           (bounds[2*i+1]-bounds[2*i]) : max);

  Error = (this->MaximumError >= VTK_LARGE_FLOAT ? 
           VTK_LARGE_FLOAT : this->MaximumError * max);
  Tolerance = VTK_TOLERANCE * input->GetLength();
  CosAngle = cos ((double) vtkMath::DegreesToRadians() * this->FeatureAngle);
  Split = ( this->Splitting && !this->PreserveTopology );
  VertexDegree = this->Degree;
  TheSplitAngle = this->SplitAngle;
  SplitState = VTK_STATE_UNSPLIT;

  // Build cell data structure. Need to copy triangle connectivity data
  // so we can modify it.
  if ( this->TargetReduction > 0.0 )
    {
    inPts = input->GetPoints();
    inPolys = input->GetPolys();
    Mesh = new vtkPolyData;
    newPts = new vtkFloatPoints(numPts); newPts->SetNumberOfPoints(numPts);
    for ( i=0; i < numPts; i++ ) newPts->SetPoint(i,inPts->GetPoint(i));
    newPolys = new vtkCellArray(*(inPolys));
    Mesh->SetPoints(newPts);
    Mesh->SetPolys(newPolys);
    newPts->Delete(); //registered by Mesh and preserved
    newPolys->Delete(); //registered by Mesh and preserved
    Mesh->BuildLinks();
    }
  else
    {
    this->Output->CopyStructure(this->Input);
    this->Output->GetPointData()->PassData(this->Input->GetPointData());
    vtkWarningMacro(<<"Reduction == 0: passing data through unchanged");
    return;
    }

  // Initialize data structures: priority queue and errors.
  this->InitializeQueue(numPts);

  if ( this->AccumulateError )
    {
    this->VertexError = new vtkFloatArray(numPts,(int) ((float)0.25*numPts));
    for (i=0; i<numPts; i++) this->VertexError->SetValue(i, 0.0);
    }

  // If not deferring splitting and splitting on, we'll start off by 
  // splitting the mesh. This has side effect of inserting vertices.
  NumCollapses = NumMerges = 0;
  if ( Split && this->PreSplitMesh )
    {
    vtkDebugMacro(<<"Pre-splitting mesh");
    SplitState = VTK_STATE_SPLIT;
    this->SplitMesh();
    }

  // Start by traversing all vertices. For each vertex, evaluate the
  // local topology/geometry. (Some vertex splitting may be
  // necessary to resolve non-manifold geometry or to split edges.) 
  // Then evaluate the local error for the vertex. The vertex is then
  // inserted into the priority queue.
  for ( ptId=0; ptId < Mesh->GetNumberOfPoints(); ptId++ )
    {
    if ( ! (ptId % 10000) ) vtkDebugMacro(<<"Inserting vertex #" << ptId);
    this->Insert(ptId);
    }

  // While the priority queue is not empty, retrieve the top vertex from the
  // queue and attempt to delete it by performing an edge collapse. This 
  // in turn will cause modification to the surrounding vertices. For each
  // surrounding vertex, evaluate the error and re-insert into the queue. 
  // (While this is happening we keep track of operations on the data - 
  // this forms the core of the progressive mesh representation.)
  for ( totalEliminated=0, reduction=0.0, numRecycles=0, numPops=0;
  reduction < this->TargetReduction && (ptId = this->Pop(error)) >= 0; 
  numPops++)
    {
    if ( ! (numPops % 10000) )
      vtkDebugMacro(<<"Deleting vertex #" << numPops);

    Mesh->GetPoint(ptId,X);
    Mesh->GetPointCells(ptId,ncells,cells);

    if ( ncells > 0 )
      {
      type = EvaluateVertex(ptId, ncells, cells, fedges);

      // FindSplit finds the edge to collapse - and if it fails, we
      // split the vertex.
      collapseId = FindSplit (type, fedges, pt1, pt2, CollapseTris);

      if ( collapseId >= 0 )
        {
        if ( this->AccumulateError ) this->DistributeError(error);

        totalEliminated += CollapseEdge(type, ptId, collapseId, pt1, pt2, 
                                        CollapseTris);

        reduction = (float) totalEliminated / numTris;
        NumberOfRemainingTris = numTris - totalEliminated;

        //see whether we've found inflection
        if ( numPops == 0 || (previousError == 0.0 && error != 0.0) ||
        (previousError != 0.0 && 
        fabs(error/previousError) > this->InflectionPointRatio) )
          {
          this->InflectionPoints.InsertNextValue(numPops);
          }
        previousError = error;
        }

      else //Couldn't delete the vertex, so we'll re-insert it for splitting
        { 
        numRecycles++;
        this->Insert(ptId,VTK_RECYCLE_VERTEX);
        }

      }//if cells attached
    }//while queue not empty and reduction not satisfied

  vtkDebugMacro(<<"\n\tReduction " << reduction << " (" << numTris << " to " 
                << numTris - totalEliminated << " triangles)"
                <<"\n\tPerformed " << numPops << " vertex pops"
                <<"\n\tFound " << this->GetNumberOfInflectionPoints() 
                <<" inflection points"
                <<"\n\tPerformed " 
                    << Mesh->GetNumberOfPoints() - numPts << " vertex splits"
                <<"\n\tPerformed " << NumCollapses << " edge collapses"
                <<"\n\tPerformed " << NumMerges << " vertex merges"
                <<"\n\tRecycled " << numRecycles << " points"
                <<"\n\tAdded " << Mesh->GetNumberOfPoints() - numPts << " points (" 
                    << numPts << " to " << Mesh->GetNumberOfPoints() << " points)");


  // Generate output at the given reduction level.
  this->DeleteQueue();

  // Now grab the cells that are left
  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(3,numTris-totalEliminated));

  for (cellId=0; cellId < numTris; cellId++)
    {
    if ( Mesh->GetCellType(cellId) == VTK_TRIANGLE ) // non-null element
      {
      Mesh->GetCellPoints(cellId, npts, pts);
      newPolys->InsertNextCell(npts,pts);
      }
    }

  output->SetPoints(Mesh->GetPoints());
  output->SetPolys(newPolys);

  Mesh->Delete();
}

//
// Computes error to edge (distance squared)
//
static float ComputeEdgeError(float x[3], float x1[3], float x2[3])
{
  float projDist = vtkLine::DistanceToLine(x, x1, x2);
  float edgeLength = vtkMath::Distance2BetweenPoints(x1,x2);

  return (projDist < edgeLength ? projDist : edgeLength);
}

//
// Computes triangle area
//
static float ComputeSingleTriangleError(float x[3], float x1[3], float x2[3])
{
  return vtkTriangle::TriangleArea(x, x1, x2);
}

//
// Computes error to a cycle of triangles...the average plane (normal and
// point) have been already computed. (Returns distance squared.)
//
static float ComputeSimpleError(float x[3], float normal[3], float point[3])
{
  float dist = vtkPlane::DistanceToPlane(x, normal, point);
  return dist * dist;
}

// 
// Split the mesh along sharp edges - separates the mesh into pieces.
//
void vtkDecimatePro::SplitMesh()
{
  int ptId, *cells, type, fedges[2];
  unsigned short int ncells;

  CosAngle = cos ((double) vtkMath::DegreesToRadians() * this->SplitAngle);
  for ( ptId=0; ptId < Mesh->GetNumberOfPoints(); ptId++ )
    {
    Mesh->GetPoint(ptId,X);
    Mesh->GetPointCells(ptId,ncells,cells);

    if ( ncells > 0 && 
    ((type=this->EvaluateVertex(ptId,ncells,cells,fedges)) == VTK_CORNER_VERTEX ||
    type == VTK_INTERIOR_EDGE_VERTEX || type == VTK_NON_MANIFOLD_VERTEX) )
      {
      this->SplitVertex(ptId, type, ncells, cells, 0);
      }
    }
}

#define FEATURE_ANGLE(tri1,tri2) \
                      vtkMath::Dot(T->Array[tri1].n, T->Array[tri2].n)
//
// Evalute the local topology/geometry of a vertex. This is a two-pass
// process: first topology is examined, and then the geometry.
//
int vtkDecimatePro::EvaluateVertex(int ptId, unsigned short int numTris, int *tris,
                          int fedges[2])
{
  int numVerts, numNei, numFEdges;
  static vtkIdList nei(VTK_MAX_TRIS_PER_VERTEX);
  vtkLocalTri t;
  vtkLocalVertex sn;
  int startVertex, nextVertex;
  int i, j, *verts, numNormals, vtype;
  float *x1, *x2, *normal;
  float v1[3], v2[3], center[3];
//
//  The first step is to evaluate topology.
//

// Check cases with high vertex degree
//
  if ( numTris >= VertexDegree ) 
    {
    return VTK_HIGH_DEGREE_VERTEX;
    }

//  From the adjacency structure we can find the triangles that use the
//  vertex. Traverse this structure, gathering all the surrounding vertices
//  into an ordered list.
//
  V->Reset();
  T->Reset();

  sn.FAngle = 0.0;

  t.n[0] = t.n[1] = t.n[2] = 0.0;
  t.verts[0] = -1; // Marks the fact that this poly hasn't been replaced 
//
//  Find the starting edge.  Do it very carefully do make sure
//  ordering is consistent (e.g., polygons ordering/normals remains consistent)
//
  Mesh->GetCellPoints(*tris,numVerts,verts); // get starting point
  for (i=0; i<3; i++) if (verts[i] == ptId) break;
  sn.id = startVertex = verts[(i+1)%3];
  Mesh->GetPoint(sn.id, sn.x); //grab coordinates here to save GetPoint() calls

  V->InsertNextVertex(sn);

  nextVertex = -1; // initialize
  nei.Reset();
  nei.InsertId(0,*tris);
  numNei = 1;
//
//  Traverse the edge neighbors and see whether a cycle can be
//  completed.  Also have to keep track of orientation of faces for
//  computing normals.
//
  while ( T->MaxId < numTris && numNei == 1 && nextVertex != startVertex) 
    {
    t.id = nei.GetId(0);
    T->InsertNextTriangle(t);

    Mesh->GetCellPoints(t.id,numVerts,verts);
        
    for (j=0; j<3; j++) 
      {
      if (verts[j] != sn.id && verts[j] != ptId) 
        {
        nextVertex = verts[j];
        break;
        }
      }
    sn.id = nextVertex;
    Mesh->GetPoint(sn.id, sn.x);
    V->InsertNextVertex(sn);

    Mesh->GetCellEdgeNeighbors(t.id, ptId, nextVertex, nei);
    numNei = nei.GetNumberOfIds();
    } 
//
//  See whether we've run around the loop, hit a boundary, or hit a
//  complex spot.
//
  if ( nextVertex == startVertex && numNei == 1 ) 
    {
    if ( T->GetNumberOfTriangles() != numTris ) //touching non-manifold
      {
      vtype = VTK_NON_MANIFOLD_VERTEX;
      } 
    else  //remove last vertex addition
      {
      V->MaxId -= 1;
      vtype = VTK_SIMPLE_VERTEX;
      }
    }
//
//  Check for non-manifold cases
//
  else if ( numNei > 1 || T->GetNumberOfTriangles() > numTris ) 
    {
    vtype = VTK_NON_MANIFOLD_VERTEX;
    }
//
//  Boundary loop - but (luckily) completed semi-cycle
//
  else if ( numNei == 0 && T->GetNumberOfTriangles() == numTris ) 
    {
    V->Array[0].FAngle = -1.0; // using cosine of -180 degrees
    V->Array[V->MaxId].FAngle = -1.0;
    vtype = VTK_BOUNDARY_VERTEX;
    }
//
//  Hit a boundary but didn't complete semi-cycle.  Gotta go back
//  around the other way.  Just reset the starting point and go 
//  back the other way.
//
  else 
    {
    t = T->GetTriangle(T->MaxId);

    V->Reset();
    T->Reset();

    startVertex = sn.id = nextVertex;
    Mesh->GetPoint(sn.id, sn.x);
    V->InsertNextVertex(sn);

    nextVertex = -1;
    nei.Reset();
    nei.InsertId(0,t.id);
    numNei = 1;
//
//  Now move from boundary edge around the other way.
//
    while ( T->MaxId < numTris && numNei == 1 && nextVertex != startVertex) 
      {
      t.id = nei.GetId(0);
      T->InsertNextTriangle(t);

      Mesh->GetCellPoints(t.id,numVerts,verts);
  
      for (j=0; j<3; j++) 
        {
        if (verts[j] != sn.id && verts[j] != ptId) 
          {
          nextVertex = verts[j];
          break;
          }
        }

      sn.id = nextVertex;
      Mesh->GetPoint(sn.id, sn.x);
      V->InsertNextVertex(sn);

      Mesh->GetCellEdgeNeighbors(t.id, ptId, nextVertex, nei);
      numNei = nei.GetNumberOfIds();
      }
//
//  Make sure that there are only two boundaries (i.e., not non-manifold)
//
    if ( T->GetNumberOfTriangles() == numTris ) 
      {
//
//  Because we've reversed order of loop, need to rearrange the order
//  of the vertices and polygons to preserve consistent polygons
//  ordering / normal orientation.
//
      numVerts = V->GetNumberOfVertices();
      for (i=0; i<(numVerts/2); i++) 
        {
        sn.id = V->Array[i].id;
        V->Array[i].id = V->Array[numVerts-i-1].id;
        V->Array[numVerts-i-1].id = sn.id;
        for (j=0; j<3; j++)
          {
          sn.x[j] = V->Array[i].x[j];
          V->Array[i].x[j] = V->Array[numVerts-i-1].x[j];
          V->Array[numVerts-i-1].x[j] = sn.x[j];
          }
        }

      numTris = T->GetNumberOfTriangles();
      for (i=0; i<(numTris/2); i++) 
        {
        t.id = T->Array[i].id;
        T->Array[i].id = T->Array[numTris-i-1].id;
        T->Array[numTris-i-1].id = t.id;
        }

      V->Array[0].FAngle = -1.0;
      V->Array[V->MaxId].FAngle = -1.0;
      vtype = VTK_BOUNDARY_VERTEX;
      } 
    else // non-manifold
      {
      vtype = VTK_NON_MANIFOLD_VERTEX;
      }
    }
//
// If at this point, the vertex is either simple or boundary. Here we do
// a geometric evaluation to find feature edges, if any, and then a
// final classification.
//

//
//  Traverse all polygons and generate normals and areas
//
  x2 =  V->Array[0].x;
  for (i=0; i<3; i++) v2[i] = x2[i] - X[i];

  LoopArea=0.0;
  Normal[0] = Normal[1] = Normal[2] = 0.0;
  Pt[0] = Pt[1] = Pt[2] = 0.0;
  numNormals=0;

  for (i=0; i < T->GetNumberOfTriangles(); i++) 
    {
    normal = T->Array[i].n;
    x1 = x2;
    x2 = V->Array[i+1].x;

    for (j=0; j<3; j++) 
      {
      v1[j] = v2[j];
      v2[j] = x2[j] - X[j];
      }

    T->Array[i].area = vtkTriangle::TriangleArea (X, x1, x2);
    vtkTriangle::TriangleCenter (X, x1, x2, center);
    LoopArea += T->Array[i].area;

    vtkMath::Cross (v1, v2, normal);
//
//  Get normals.  If null, then normal make no contribution to loop.
//  The center of the loop is the center of gravity.
//
    if ( vtkMath::Normalize(normal) != 0.0 ) 
      {
      numNormals++;
      for (j=0; j<3; j++) 
        {
        Normal[j] += T->Array[i].area * normal[j];
        Pt[j] += T->Array[i].area * center[j];
        }
      }
    }
//
//  Compute "average" plane normal and plane center.  Use an area
//  averaged normal calulation
//
  if ( !numNormals || LoopArea == 0.0 ) 
    {
    return VTK_DEGENERATE_VERTEX;
    }

  for (j=0; j<3; j++) 
    {
    Normal[j] /= LoopArea;
    Pt[j] /= LoopArea;
    }
  if ( vtkMath::Normalize(Normal) == 0.0 )
    {
    return VTK_DEGENERATE_VERTEX;
    }
//
//  Now run through polygons again generating feature angles.  (Note
//  that if an edge is on the boundary its feature angle has already
//  been set to 180.)  Also need to keep track whether any feature
//  angles exceed the current value.
//
  if ( vtype == VTK_BOUNDARY_VERTEX ) 
    {
    numFEdges = 2;
    fedges[0] = 0;
    fedges[1] = V->MaxId;
    } 
  else
    numFEdges = 0;
//
//  Compare to cosine of feature angle to avoid cosine extraction
//
  if ( vtype == VTK_SIMPLE_VERTEX ) // first edge 
    if ( (V->Array[0].FAngle = FEATURE_ANGLE(0,T->MaxId)) <= CosAngle )
      fedges[numFEdges++] = 0;

  for (i=0; i < T->MaxId; i++) 
    {
    if ( (V->Array[i+1].FAngle = FEATURE_ANGLE(i,i+1)) <= CosAngle ) 
      {
      if ( numFEdges >= 2 ) 
        numFEdges++;
      else 
        fedges[numFEdges++] = i + 1;
      }
    }
//
//  Final classification
//
  if ( vtype == VTK_SIMPLE_VERTEX && numFEdges > 0 )
    {
    if ( numFEdges == 1 ) vtype = VTK_EDGE_END_VERTEX;
    else if ( numFEdges == 2 ) vtype = VTK_INTERIOR_EDGE_VERTEX;
    else vtype = VTK_CORNER_VERTEX;
    }
  else if ( vtype == VTK_BOUNDARY_VERTEX )
    {
    if ( numFEdges != 2 ) vtype = VTK_CORNER_VERTEX;
    else
      {//see whether this is the tip of a crack
      if ( V->Array[fedges[0]].x[0] == V->Array[fedges[1]].x[0] &&
      V->Array[fedges[0]].x[1] == V->Array[fedges[1]].x[1] && 
      V->Array[fedges[0]].x[2] == V->Array[fedges[1]].x[2])
        {
        vtype = VTK_CRACK_TIP_VERTEX;
        }
      }
    }

  return vtype;
}

//
// Split the vertex by modifying topological connections.
//
void vtkDecimatePro::SplitVertex(int ptId, int type, unsigned short int numTris, int *tris,
                        int insert)

{
  int i, j, id, fedge1, fedge2;
  int nverts, *verts, tri, numSplitTris, veryFirst;
  float error;
  int startTri, p[2], maxGroupSize;

  //
  // On an interior edge split along the edge
  //
  if ( type == VTK_INTERIOR_EDGE_VERTEX ) //when edge splitting is on
    {
    // Half of loop is left connected to current vertex. Second half is
    // split away.
    for ( i=0; i < numTris; i++ ) // find first feature edge
      if ( V->Array[i].FAngle <= CosAngle ) break;

    fedge1 = i;
    for ( i++, numSplitTris=1; V->Array[i].FAngle > CosAngle; i++ ) 
        numSplitTris++;
    fedge2 = i;

    // Now split region
    id = Mesh->InsertNextLinkedPoint(X,numSplitTris);

    for ( i=fedge1; i < fedge2; i++ )
      { //disconnect from existing vertex
      tri = T->Array[i].id;
      Mesh->RemoveReferenceToCell(ptId, tri);
      Mesh->AddReferenceToCell(id, tri);
      Mesh->ReplaceCellPoint(tri, ptId, id);
      }

    // Compute error and insert the two vertices (old + split)
    error = ComputeEdgeError(X, V->Array[fedge1].x, V->Array[fedge2].x);
    if ( this->AccumulateError ) 
      this->VertexError->InsertValue(id, this->VertexError->GetValue(ptId));

    if ( insert )
      {
      this->Insert(ptId,error);
      this->Insert(id,error);
      }
    }

  //
  // Break corners into separate pieces (along feature edges)
  //
  else if ( type == VTK_CORNER_VERTEX ) 
    {
    // The first piece is left connected to vertex. Just find first 
    // feature/boundary edge. If on boundary, skip boundary piece.
    for ( i=0; i <= V->MaxId; i++ ) // find first feature edge
      if ( V->Array[i].FAngle <= CosAngle && V->Array[i].FAngle != -1.0 ) 
        break;

    for ( veryFirst = fedge1 = i; fedge1 < V->MaxId; i = fedge1 = fedge2 )
      {
      for (i++, numSplitTris=1; 
      i <= V->MaxId && V->Array[i].FAngle > CosAngle; i++) numSplitTris++;

      if ( (fedge2 = i) > V->MaxId ) continue; //must be part of first region

      // Now split region
      id = Mesh->InsertNextLinkedPoint(X,numSplitTris);
      for ( j=fedge1; j < fedge2; j++ )
        { //disconnect from existing vertex
        tri = T->Array[j].id;
        Mesh->RemoveReferenceToCell(ptId, tri);
        Mesh->AddReferenceToCell(id, tri);
        Mesh->ReplaceCellPoint(tri, ptId, id);
        }

      // Compute error for the vertex and insert
      error = ComputeEdgeError(X, V->Array[fedge1].x, V->Array[fedge2].x);
      if ( this->AccumulateError ) 
        this->VertexError->InsertValue(id, this->VertexError->GetValue(ptId));

      if ( insert ) this->Insert(id,error);    
      }

    // don't forget to compute error for old vertex, and insert into queue
    if ( V->Array[0].FAngle == -1.0 )
      error = ComputeEdgeError(X, V->Array[0].x, V->Array[veryFirst].x);
    else
      error = ComputeEdgeError(X, V->Array[veryFirst].x, V->Array[fedge1].x);

    if ( insert ) this->Insert(ptId,error);    
    }

  // Default case just splits off triangle(s) that form manifold groups. 
  // Note: this code also handles high-degree vertices.
  else
    {
    vtkIdList triangles(VTK_MAX_TRIS_PER_VERTEX);
    vtkIdList cellIds(5,10);
    vtkIdList group(VTK_MAX_TRIS_PER_VERTEX);

     //changes in group size control how to split loop
    if ( numTris <= 1 ) return; //prevents infinite recursion
    maxGroupSize = ( numTris < VertexDegree ? numTris : (VertexDegree - 1));

    if ( type == VTK_NON_MANIFOLD_VERTEX || type == VTK_HIGH_DEGREE_VERTEX )
      ; //use maxGroupSize
    else
      maxGroupSize /= 2; //prevents infinite recursion

    for ( i=0; i < numTris; i++ ) triangles.InsertId(i,tris[i]);

    // now group into manifold pieces
    for ( i=0, id=ptId; triangles.GetNumberOfIds() > 0; i++ )
      {
      group.Reset();
      startTri = triangles.GetId(0);
      group.InsertId(0,startTri);
      triangles.DeleteId(startTri);
      Mesh->GetCellPoints(startTri,nverts,verts);
      p[0] = ( verts[0] != ptId ? verts[0] : verts[1] );
      p[1] = ( verts[1] != ptId && verts[1] != p[0] ? verts[1] : verts[2] );

      //grab manifold group - j index is the forward/backward direction around vertex
      for ( j=0; j < 2; j++ )
        {
        for ( tri=startTri; p[j] >= 0; )
          {
          Mesh->GetCellEdgeNeighbors(tri, ptId, p[j], cellIds);
          if ( cellIds.GetNumberOfIds() == 1 && triangles.IsId((tri=cellIds.GetId(0)))
          && group.GetNumberOfIds() < maxGroupSize )
            {
            group.InsertNextId(tri);
            triangles.DeleteId(tri);
            Mesh->GetCellPoints(tri,nverts,verts);
            if ( verts[0] != ptId && verts[0] != p[j] ) p[j] = verts[0];
            else if ( verts[1] != ptId && verts[1] != p[j] ) p[j] = verts[1];
            else p[j] = verts[2];
            }
          else p[j] = -1;
          }
        }//for both directions

      // reconnect group into manifold chunk (first group is left attached)
      if ( i != 0 ) 
        {
        id = Mesh->InsertNextLinkedPoint(X,group.GetNumberOfIds());
        for ( j=0; j < group.GetNumberOfIds(); j++ )
          {
          tri = group.GetId(j);
          Mesh->RemoveReferenceToCell(ptId, tri);
          Mesh->AddReferenceToCell(id, tri);
          Mesh->ReplaceCellPoint(tri, ptId, id);
          }
        if ( this->AccumulateError ) 
          this->VertexError->InsertValue(id, this->VertexError->GetValue(ptId));
        if ( insert ) this->Insert(id);
        }//if not first group
      }//for all groups
    //Don't forget to reinsert original vertex
    if ( insert ) this->Insert(ptId);
    }

  return;
}


//
// Find a way to split this loop. If -1 is returned, then we have a real
// bad situation and we'll split the vertex.
//
int vtkDecimatePro::FindSplit (int type, int fedges[2], int& pt1, int& pt2, 
                      vtkIdList& CollapseTris)
{
  int i, maxI;
  float dist2, e2dist2;
  int numVerts=V->MaxId+1;
  static vtkPriorityQueue EdgeLengths(VTK_MAX_TRIS_PER_VERTEX);

  CollapseTris.SetNumberOfIds(2);
  EdgeLengths.Reset();

  switch (type)
    {

    case VTK_SIMPLE_VERTEX: case VTK_EDGE_END_VERTEX: case VTK_INTERIOR_EDGE_VERTEX:
      if ( type == VTK_INTERIOR_EDGE_VERTEX )
        {
        dist2 = vtkMath::Distance2BetweenPoints(X, V->Array[fedges[0]].x);
        EdgeLengths.Insert(dist2,fedges[0]);

        dist2 = vtkMath::Distance2BetweenPoints(X, V->Array[fedges[1]].x);
        EdgeLengths.Insert(dist2,fedges[1]);
        }
      else // Compute the edge lengths
        {
        for ( i=0; i < numVerts; i++ )
          {
          dist2 = vtkMath::Distance2BetweenPoints(X, V->Array[i].x);
          EdgeLengths.Insert(dist2,i);
          }
        }

      // See whether the collapse is okay
      while ( (maxI = EdgeLengths.Pop(dist2)) >= 0 )
        {
        if ( IsValidSplit(maxI) ) break;
        }

      if ( maxI >= 0 )
        {
        CollapseTris.SetId(0,T->Array[maxI].id);
        if ( maxI == 0 )
          {
          pt1 = V->Array[1].id;
          pt2 = V->Array[V->MaxId].id;
          CollapseTris.SetId(1,T->Array[T->MaxId].id);
          }
        else
          {
          pt1 = V->Array[(maxI+1)%numVerts].id;
          pt2 = V->Array[maxI-1].id;
          CollapseTris.SetId(1,T->Array[maxI-1].id);
          }

        return V->Array[maxI].id;
        }
      break;


    case VTK_BOUNDARY_VERTEX: //--------------------------------------------
      CollapseTris.SetNumberOfIds(1);
      // Compute the edge lengths
      dist2 = vtkMath::Distance2BetweenPoints(X, V->Array[0].x);
      e2dist2 = vtkMath::Distance2BetweenPoints(X, V->Array[V->MaxId].x);

      maxI = -1;
      if ( dist2 <= e2dist2 )
        {
        if ( IsValidSplit(0) ) maxI = 0;
        else if ( IsValidSplit(V->MaxId) ) maxI = V->MaxId;
        }
      else
        {
        if ( IsValidSplit(V->MaxId) ) maxI = V->MaxId;
        else if ( IsValidSplit(0) ) maxI = 0;
        }

      if ( maxI >= 0 )
        {
        if ( maxI == 0 )
          {
          CollapseTris.SetId(0,T->Array[0].id);
          pt1 = V->Array[1].id;
          return V->Array[0].id;
          }
        else
          {
          CollapseTris.SetId(0,T->Array[T->MaxId].id);
          pt1 = V->Array[V->MaxId-1].id;
          return V->Array[V->MaxId].id;
          }
        }
      break;


    case VTK_CRACK_TIP_VERTEX: //-------------------------------------------
      V->MaxId--;
      if ( IsValidSplit(0) )
        {
        CollapseTris.SetId(0,T->Array[0].id);
        pt1 = V->Array[1].id;
        pt2 = V->Array[V->MaxId].id;
        CollapseTris.SetId(1,T->Array[T->MaxId].id);
        return V->Array[0].id;
        }
      else V->MaxId++;
      break;


    case VTK_DEGENERATE_VERTEX: //-------------------------------------------
      // Collapse to the first edge
      CollapseTris.SetId(0,T->Array[0].id);
      pt1 = V->Array[1].id;
      if ( T->MaxId > 0 ) //more than one triangle
        {
        if ( T->MaxId == V->MaxId ) //a complete cycle
          {
          CollapseTris.SetId(1,T->Array[T->MaxId].id);
          pt2 = V->Array[V->MaxId].id;
          }
        }

      return V->Array[0].id;


    default:
      ;
    }

  return -1;
}

//
//  Determine whether the loop can be split at the vertex indicated
//
int vtkDecimatePro::IsValidSplit(int index)
{
  int i, j, sign, fedges[2];
  int nverts=V->MaxId+1;
  float *x, val, absVal, sPt[3], v21[3], sN[3];
  int l1[VTK_MAX_TRIS_PER_VERTEX], l2[VTK_MAX_TRIS_PER_VERTEX];
  int n1, n2;

  // For a edge collapse to be valid, all edges to that vertex must
  // divide the loop cleanly.
  fedges[0] = index;
  for ( j=0; j < (nverts-3); j++ )
    {
    fedges[1] = (index + 2 + j) % nverts;
    SplitLoop (fedges, n1, l1, n2, l2);

    //  Create splitting plane.  Splitting plane is parallel to the loop
    //  plane normal and contains the splitting vertices fedges[0] and fedges[1].
    for (i=0; i<3; i++) 
      {
      sPt[i] = V->Array[fedges[0]].x[i];
      v21[i] = V->Array[fedges[1]].x[i] - sPt[i];
      }

    vtkMath::Cross (v21,Normal,sN);
    if ( vtkMath::Normalize(sN) == 0.0 ) return 0;

    for (sign=0, i=0; i < n1; i++) // first loop 
      {
      if ( !(l1[i] == fedges[0] || l1[i] == fedges[1]) ) 
        {
        x = V->Array[l1[i]].x;
        val = vtkPlane::Evaluate(sN,sPt,x);
        if ( (absVal = (float) fabs((double)val)) < Tolerance ) return 0;
        
        if ( !sign )
          sign = (val > Tolerance ? 1 : -1);
        else if ( sign != (val > 0 ? 1 : -1) )
          return 0;
        }
      }

    sign *= -1;
    for (i=0; i < n2; i++) // second loop 
      {
      if ( !(l2[i] == fedges[0] || l2[i] == fedges[1]) ) 
        {
        x = V->Array[l2[i]].x;
        val = vtkPlane::Evaluate(sN,sPt,x);
        if ( (absVal = (float) fabs((double)val)) < Tolerance ) return 0;
        
        if ( !sign )
          sign = (val > Tolerance ? 1 : -1);
        else if ( sign != (val > 0 ? 1 : -1) )
          return 0;
        }
      }
    }// Check all splits
  return 1;
}

//
//  Creates two loops from splitting plane provided
//
void vtkDecimatePro::SplitLoop(int fedges[2], int& n1, int *l1, int& n2, int *l2)
{
  int i;
  int *loop;
  int *count;

  n1 = n2 = 0;
  loop = l1;
  count = &n1;

  for (i=0; i <= V->MaxId; i++) 
    {
    loop[(*count)++] = i;
    if ( i == fedges[0] || i == fedges[1] ) 
      {
      loop = (loop == l1 ? l2 : l1);
      count = (count == &n1 ? &n2 : &n1);
      loop[(*count)++] = i;
      }
    }
}

// Collapse the point to the specified vertex. Distribute the error
// and update neighborhood vertices.
int vtkDecimatePro::CollapseEdge(int type, int ptId, int collapseId, int pt1, 
                 int pt2, vtkIdList& CollapseTris)
{
  int i, numDeleted=CollapseTris.GetNumberOfIds();
  int ntris=T->MaxId+1;
  int nverts=V->MaxId+1;
  int tri[2];
  int verts[VTK_MAX_TRIS_PER_VERTEX+1];

  NumCollapses++;
  for ( i=0; i < numDeleted; i++ ) tri[i] = CollapseTris.GetId(i);

  if ( numDeleted == 2 ) // type == VTK_CRACK_TIP_VERTEX || type == VTK_SIMPLE_VERTEX
    {
    if ( type == VTK_CRACK_TIP_VERTEX ) //got to seal the crack first
      {
      NumMerges++;
      Mesh->RemoveReferenceToCell(V->Array[V->MaxId+1].id, tri[1]);
      Mesh->ReplaceCellPoint(tri[1],V->Array[V->MaxId+1].id, collapseId);
      }

    // delete two triangles
    Mesh->RemoveReferenceToCell(pt1, tri[0]);
    Mesh->RemoveReferenceToCell(pt2, tri[1]);
    Mesh->RemoveReferenceToCell(collapseId, tri[0]);
    Mesh->RemoveReferenceToCell(collapseId, tri[1]);
    Mesh->DeletePoint(ptId);
    Mesh->DeleteCell(tri[0]); Mesh->DeleteCell(tri[1]);

    // update topology to reflect new attachments
    Mesh->ResizeCellList(collapseId, ntris-2);
    for ( i=0; i < ntris; i++ )
      {
      if ( T->Array[i].id != tri[0] && T->Array[i].id != tri[1] )
        {
        Mesh->AddReferenceToCell(collapseId, T->Array[i].id);
        Mesh->ReplaceCellPoint(T->Array[i].id,ptId,collapseId);
        }
      }
    }//if interior vertex

  else // if ( numDeleted == 1 ) e.g., VTK_BOUNDARY_VERTEX
    {
    // delete one triangle
    Mesh->RemoveReferenceToCell(pt1, tri[0]);
    Mesh->RemoveReferenceToCell(collapseId, tri[0]);
    Mesh->DeletePoint(ptId);
    Mesh->DeleteCell(tri[0]);

    // update topology to reflect new attachments
    if ( ntris > 1 )
      {
      Mesh->ResizeCellList(collapseId, ntris-1);
      for ( i=0; i < ntris; i++ )
        {
        if ( T->Array[i].id != tri[0] )
          {
          Mesh->AddReferenceToCell(collapseId, T->Array[i].id);
          Mesh->ReplaceCellPoint(T->Array[i].id,ptId,collapseId);
          }
        }
      }
    } //else boundary vertex

  // Update surrounding vertices. Need to copy verts first because the V/T
  // arrays might change as points are being reinserted.
  //
  for ( i=0; i < nverts; i++ ) verts[i] = V->Array[i].id;
  for ( i=0; i < nverts; i++ )
    {
    this->Delete(verts[i]);
    this->Insert(verts[i]);
    }

  return numDeleted;
}

// Description:
// Get a list of inflection points. These are float values 0 < r <= 1.0 
// corresponding to reduction level, and there are a total of
// NumberOfInflectionPoints() values. You must provide an array (of
// the correct size) into which the inflection points are written.
void vtkDecimatePro::GetInflectionPoints(float *inflectionPoints)
{
  int i;

  for (i=0; i < this->GetNumberOfInflectionPoints(); i++)
    {
    inflectionPoints[i] = this->InflectionPoints.GetValue(i);
    }
}

// Description:
// Get a list of inflection points. These are float values 0 < r <= 1.0 
// corresponding to reduction level, and there are a total of
// NumberOfInflectionPoints() values. You must provide an array (of
// the correct size) into which the inflection points are written.
// This method returns a pointer to a list of inflection points.
float *vtkDecimatePro::GetInflectionPoints()
{
  return this->InflectionPoints.GetPtr(0);
}

// Description:
// Get the number of inflection points. Only returns a valid value
// after the filter has executed.
int vtkDecimatePro::GetNumberOfInflectionPoints()
{
  return this->InflectionPoints.GetMaxId()+1;
}

//
// The following are private functions used to manage the priority
// queue of vertices.
//

void vtkDecimatePro::InitializeQueue(int numPts)
{
  if ( !this->PreserveTopology && this->Splitting ) 
    numPts = (int) ((float)numPts*1.25);

  this->Queue = new vtkPriorityQueue(numPts, (int)((float)0.25*numPts));
}

int vtkDecimatePro::Pop(float &error)
{
  int ptId;

  // Try returning what's in queue
  if ( (ptId = this->Queue->Pop(error)) >= 0 )
    {
    if ( error > Error ) this->Queue->Reset();
    else return ptId;
    }

  // See whether anything's left and split/re-insert if allowed
  if ( NumberOfRemainingTris > 0 && Split && SplitState == VTK_STATE_UNSPLIT )
    {
    vtkDebugMacro(<<"Splitting mesh");

    SplitState = VTK_STATE_SPLIT;
    this->SplitMesh();
    CosAngle = cos ((double) vtkMath::DegreesToRadians() * this->SplitAngle);

    // Now that things are split, insert the vertices. (Have to do this
    // otherwise error calculation is incorrect.)
    for ( ptId=0; ptId < Mesh->GetNumberOfPoints(); ptId++ )
      {
      this->Insert(ptId);
      }

    if ( (ptId = this->Queue->Pop(error)) >= 0 )
      {
      if ( error > Error ) this->Queue->Reset();
      else return ptId;
      }
    }

  // If here, then mesh splitting hasn't helped or is exhausted. Run thru
  // vertices and split them as necessary no matter what.
  if ( NumberOfRemainingTris > 0 && Split && SplitState != VTK_STATE_SPLIT_ALL )
    {
    vtkDebugMacro(<<"Final splitting attempt");

    SplitState = VTK_STATE_SPLIT_ALL;
    for ( ptId=0; ptId < Mesh->GetNumberOfPoints(); ptId++ )
      {
      this->Insert(ptId);
      }

    if ( (ptId = this->Queue->Pop(error)) >= 0 )
      {
      if ( error > Error ) this->Queue->Reset();
      else return ptId;
      }
    }

  //every possible point has been processed
  return -1; 
}

// Computes error and inserts point into priority queue.
void vtkDecimatePro::Insert(int ptId, float error)
{
  int type, *cells, simpleType;
  int fedges[2];
  unsigned short int ncells;

  //Depending on value of error, we need to compute it or just insert the point
  if ( error < -Tolerance )
    {
    Mesh->GetPoint(ptId,X);
    Mesh->GetPointCells(ptId,ncells,cells);

    if ( ncells > 0 )
      {
      simpleType = 0;
      type = EvaluateVertex(ptId, ncells, cells, fedges);

      // Compute error for simple types - split vertex handles others
      if ( type == VTK_SIMPLE_VERTEX || type == VTK_EDGE_END_VERTEX ||
      type == VTK_CRACK_TIP_VERTEX )
        {
        simpleType = 1;
        error = ComputeSimpleError(X,Normal,Pt);
        }

      else if ( (type == VTK_INTERIOR_EDGE_VERTEX) ||
      (type == VTK_BOUNDARY_VERTEX && this->BoundaryVertexDeletion) )
        {
        simpleType = 1;
        if ( ncells == 1 ) //compute better error for single triangle 
          error = ComputeSingleTriangleError(X,V->Array[0].x, V->Array[1].x);
        else
          error = ComputeEdgeError(X, V->Array[fedges[0]].x, 
                                   V->Array[fedges[1]].x);
        }

      if ( simpleType )
        {
        if ( this->AccumulateError ) error += this->VertexError->GetValue(ptId);
        this->Queue->Insert(error,ptId);
        }

      // Type is complex so we break it up (if splitting allowed). A 
      // side-effect of splitting a vertex is that it inserts it and any 
      // new vertices into queue.
      else if ( SplitState == VTK_STATE_SPLIT )
        {
        SplitVertex(ptId, type, ncells, cells, 1);
        } //not a simple type

      } //if cells attached to vertex
    } //need to compute the error

  // If point is being recycled, see whether we want to split it
  else if ( error >= VTK_RECYCLE_VERTEX )
    {
    //see whether to split it, otherwise it isn't inserted yet
    if ( SplitState == VTK_STATE_SPLIT_ALL )
      {
      Mesh->GetPoint(ptId,X);
      Mesh->GetPointCells(ptId,ncells,cells);
      if ( ncells > 0 ) 
        {
        type = EvaluateVertex(ptId, ncells, cells, fedges);
        SplitVertex(ptId, type, ncells, cells, 1);
        }
      }
    }

  // Sometimes the error is computed for us so we insert it appropriately
  else 
    {
    if ( this->AccumulateError ) error += this->VertexError->GetValue(ptId);
    this->Queue->Insert(error,ptId);
    }
}

// Compute the error of the point to the new triangulated surface
void vtkDecimatePro::DistributeError(float error)
{
  int i;
  int nverts=V->MaxId+1;
  float previousError;

  for (i=0; i < nverts; i++)
    {
    previousError = this->VertexError->GetValue(V->Array[i].id);
    this->VertexError->SetValue(V->Array[i].id, previousError+error);
    }
}

void vtkDecimatePro::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Target Reduction: " << this->TargetReduction << "\n";
  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";

  os << indent << "Splitting: " << (this->Splitting ? "On\n" : "Off\n");
  os << indent << "Split Angle: " << this->SplitAngle << "\n";
  os << indent << "Pre-Split Mesh: "  << (this->PreSplitMesh ? "On\n" : "Off\n");

  os << indent << "Degree: " << this->Degree << "\n";

  os << indent << "Preserve Topology: "  << (this->PreserveTopology ? "On\n" : "Off\n");
  os << indent << "Maximum Error: " << this->MaximumError << "\n";

  os << indent << "Boundary Vertex Deletion: "  << (this->BoundaryVertexDeletion ? "On\n" : "Off\n");

  os << indent << "Inflection Point Ratio: " << this->InflectionPointRatio << "\n";
  os << indent << "Number Of Inflection Points: " 
     << this->GetNumberOfInflectionPoints() << "\n";

}

