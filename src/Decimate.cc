/*=========================================================================

  Program:   Visualization Library
  Module:    Decimate.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Decimate.hh"
#include "vlMath.hh"
#include "Triangle.hh"
#include "Plane.hh"
#include "Polygon.hh"
#include "Line.hh"

// Description:
// Create object with target reduction of 90%, feature angle of 30 degrees, 
// criterion of 0.0 criterion increment of 0.005, and maximum iterions of 6.
vlDecimate::vlDecimate()
{
  this->FeatureAngle = 30;
  this->FeatureAngleIncrement = 0.0;
  this->MaximumFeatureAngle = 60;
  this->PreserveEdges = 1;
  this->BoundaryVertexDeletion = 1;

  this->Criterion = 0.0;
  this->CriterionIncrement = 0.005;
  this->MaximumCriterion = 0.1;

  this->TargetReduction = 0.90;

  this->MaximumIterations = 6;
  this->MaximumSubIterations = 2;

  this->AspectRatio = 25.0;

  this->Degree = MAX_CELL_SIZE;
}

#define TOLERANCE 1.0e-05

#define COMPLEX_VERTEX 0
#define SIMPLE_VERTEX 1
#define BOUNDARY_VERTEX 2
#define INTERIOR_EDGE_VERTEX 3
#define CORNER_VERTEX 4

#define ELIMINATED_DISTANCE_TO_PLANE 5
#define ELIMINATED_DISTANCE_TO_EDGE 6
#define FAILED_DEGREE_TEST 7
#define FAILED_NON_MANIFOLD 8
#define FAILED_ZERO_AREA_TEST 9
#define FAILED_ZERO_NORMAL_TEST 10
#define FAILED_TO_TRIANGULATE 11

//
// Special classes for manipulating data
//
class vlVertexArray {
public:
  vlVertexArray(const int sz) 
    {this->MaxId = -1; this->Array = new struct vlVertex[sz];};
  ~vlVertexArray() {if (this->Array) delete [] this->Array;};
  int GetNumberOfVertices() {return this->MaxId + 1;};
  int InsertNextVertex(vlVertex& v) 
    {this->MaxId++; this->Array[this->MaxId] = v;};
  struct vlVertex& GetVertex(int i) {return this->Array[i];};
  void Reset() {this->MaxId = -1;};

  struct vlVertex *Array;  // pointer to data
  int MaxId;             // maximum index inserted thus far
};

class vlTriArray {
public:
  vlTriArray(const int sz) 
    {this->MaxId = -1; this->Array = new struct vlTri[sz];};
  ~vlTriArray() {if (this->Array) delete [] this->Array;};
  int GetNumberOfTriangles() {return this->MaxId + 1;};
  int InsertNextTriangle(vlTri& t) 
    {this->MaxId++; this->Array[this->MaxId] = t;};
  struct vlTri& GetTriangle(int i) {return this->Array[i];};
  void Reset() {this->MaxId = -1;};

  struct vlTri *Array;  // pointer to data
  int MaxId;            // maximum index inserted thus far
};

#define MAX_TRIS_PER_VERTEX MAX_CELL_SIZE
#define MAX_SQUAWKS 10
//
//  Static variables visible in this file
//
static vlPlane plane; // eliminate constructor overhead
static vlLine line;
static vlTriangle triangle;
static vlMath math;

static vlPolyData *Mesh; // operate on this data structure
static float Pt[3], Normal[3]; // least squares plane point & normal
static float Angle, Distance; // current feature angle and distance 
static float CosAngle; // Cosine of dihedral angle

static float Tolerance; // Intersection tolerance
static float AspectRatio2; // Allowable aspect ratio 
static int ContinueTriangulating; // Stops recursive tri. if necessary 
static int Squawks; // Control output 

// temporary working arrays
static vlVertexArray V(2*MAX_TRIS_PER_VERTEX); //cycle of vertices around point
static vlTriArray T(2*MAX_TRIS_PER_VERTEX); //cycle of triangles around point


//
//  Reduce triangles in mesh by given amount or until total number of
//  iterations completes
//
void vlDecimate::Execute()
{
  int numPts, numTris;
  vlPoints *inPts;
  vlCellArray *inPolys;
  int numVerts;
  float *bounds, max;
  int i, ptId;
  vlCellArray *newPolys;
  float reduction=0.0;
  int iteration=0, sub;
  int trisEliminated;
  unsigned short int ncells;
  int *cells;
  int numFEdges;
  VertexPtr fedges[2];
  int vtype;
  int npts, *pts, newCellPts[MAX_CELL_SIZE];
  VertexPtr verts[MAX_TRIS_PER_VERTEX];
  VertexPtr l1[MAX_TRIS_PER_VERTEX], l2[MAX_TRIS_PER_VERTEX];
  int n1, n2, cellId;
  float ar, criterion;
  int totalEliminated=0;
  int *map, numNewPts, size;
  vlPointData *pd;
  vlFloatPoints *newPts;

  vlDebugMacro(<<"Decimating mesh...");
  this->Initialize();
//
// Check input
//
  if ( (numPts=this->Input->GetNumberOfPoints()) < 1 || 
  (numTris=this->Input->GetNumberOfPolys()) < 1 )
    {
    vlErrorMacro(<<"No data to decimate!");
    return;
    }
//
//  Get the bounds of the data to compute decimation threshold
//
  bounds = this->Input->GetBounds();

  for (max=0.0, i=0; i<3; i++)
    max = ((bounds[2*i+1]-bounds[2*i]) > max ? 
           (bounds[2*i+1]-bounds[2*i]) : max);

  Tolerance = max * TOLERANCE;
  criterion = this->Criterion;
  Distance = this->Criterion * max;
  Angle = this->FeatureAngle;
  CosAngle = cos ((double) math.DegreesToRadians() * this->FeatureAngle);
  AspectRatio2 = 1.0 / (this->AspectRatio * this->AspectRatio);
  Squawks = 0;

  vlDebugMacro(<<"Decimating " << numPts << " vertices, " << numTris 
               << " triangles with:\n"
               << "\tIterations= " << this->MaximumIterations << "\n"
               << "\tSub-iterations= " << this->MaximumSubIterations << "\n"
               << "\tLength= " << max << "\n"
               << "\tCriterion= " << this->Criterion << "\n"
               << "\tDistance= " << Distance << "\n"
               << "\tAspect ratio= " << this->AspectRatio << "\n"
               << "\tMaximum vertex degree= " << this->Degree);
//
// Build cell data structure. Need to copy triangle connectivity data
// so we can modify it.
//
  inPts = this->Input->GetPoints();
  inPolys = this->Input->GetPolys();
  Mesh = new vlPolyData;
  Mesh->SetPoints(inPts);
  newPolys = new vlCellArray(*(inPolys));
  Mesh->SetPolys(newPolys);
  Mesh->BuildLinks();
//
//  Traverse all vertices, eliminating those that meet decimation
//  criterion.  Initialize loop information.
//
//************************************ Outer Loop ***************************

  while ( reduction < this->TargetReduction && iteration < this->MaximumIterations ) 
    {
    trisEliminated = 1;

//******************************** Subiterations ****************************

    for (sub=0; sub < this->MaximumSubIterations && trisEliminated &&
    reduction < this->TargetReduction; sub++) 
      {
      for (i=0; i < NUMBER_STATISTICS; i++) this->Stats[i] = 0;
      trisEliminated = 0;
//
//  For every vertex that is used by two or more elements and has a loop
//  of simple enough complexity...
//
      for (ptId=0; ptId < numPts; ptId++)
        {
        if ( ! (ptId % 5000) ) vlDebugMacro(<<"vertex #" << ptId);

        Mesh->GetPointCells(ptId,ncells,cells);
        if ( ncells > 1 && 
        (vtype=this->BuildLoop(ptId,ncells,cells)) != COMPLEX_VERTEX )
          {
//
//  Determine the distance of the vertex to an "average plane"
//  through the loop.  If it's less than the decimation distance
//  criterion, then vertex can be eliminated.  If the vertex is on the
//  boundary, see whether it can be eliminated based on distance to
//  boundary.
//
          ContinueTriangulating = 0;
          this->EvaluateLoop (ptId, vtype, numFEdges, fedges);

          if ( vtype != COMPLEX_VERTEX ) 
            {
            ContinueTriangulating = 1;
            numVerts = V.GetNumberOfVertices();
            for (i=0; i < numVerts; i++) verts[i] = V.Array + i;
            }
//
//  Note: interior edges can be eliminated if decimation criterion met
//  and flag set.
//
          if ( (vtype == SIMPLE_VERTEX || 
          ( (vtype == INTERIOR_EDGE_VERTEX || vtype == CORNER_VERTEX) && !this->PreserveEdges)) &&
          plane.DistanceToPlane(Mesh->GetPoint(ptId),Normal,Pt) <= Distance)
            {
            this->Triangulate (numVerts, verts);
            this->Stats[ELIMINATED_DISTANCE_TO_PLANE]++;
//
//  If the vertex is on an interior edge, then see whether the
//  be eliminated based on distance to line (e.g., edge), and it the
//  loop can be split.  
//
            } 
          else if ((vtype == INTERIOR_EDGE_VERTEX || 
          vtype == BOUNDARY_VERTEX) &&
          line.DistanceToLine(Mesh->GetPoint(ptId), Mesh->GetPoint(fedges[0]->id), Mesh->GetPoint(fedges[1]->id)) <= (Distance*Distance) &&
          this->CanSplitLoop (fedges,numVerts,verts,n1,l1,n2,l2,ar) )
            {
            this->Triangulate (n1, l1);
            this->Triangulate (n2, l2);
            this->Stats[ELIMINATED_DISTANCE_TO_EDGE]++;
            } 
          else
            {
            ContinueTriangulating = 0;
            }

          if ( ContinueTriangulating ) 
            {
            if ( vtype == BOUNDARY_VERTEX ) trisEliminated += 1;
            else trisEliminated += 2;

            // Update the data structure to reflect deletion of vertex
            Mesh->DeletePoint(ptId);
            for (i=0; i < V.GetNumberOfVertices(); i++)
              if ( (size=V.Array[i].newRefs-V.Array[i].deRefs) > 0 )
                Mesh->ResizeCellList(V.Array[i].id,size);

            for (i=0; i < T.GetNumberOfTriangles(); i++)
              Mesh->RemoveCellReference(T.Array[i].id);

            for (i=0; i < T.GetNumberOfTriangles(); i++)
              if ( T.Array[i].verts[0] != -1 ) //replaced with new triangle
                Mesh->ReplaceLinkedCell(T.Array[i].id, 3, T.Array[i].verts);
              else
                Mesh->DeleteCell(T.Array[i].id);

            }
          }
        }

      totalEliminated += trisEliminated;
      reduction = (float) totalEliminated / numTris;

      vlDebugMacro(<<"\n\tIteration = " << iteration+1 << "\n"
                   <<"\tSub-iteration = " << sub+1 << "\n"
                   <<"\tPolygons removed = " << trisEliminated << "\n"
                   <<"\tTotal removed = " << totalEliminated << "\n"
                   <<"\tRemaining = " << numTris - totalEliminated << "\n"
                   <<"\tOriginal triangles = " << numTris << "\n"
                   <<"\tReduction = " << reduction << "\n"
                   <<"\tCriterion = " << criterion << "\n"
                   <<"\tDistance = " << Distance << "\n"
                   <<"\tFeature angle = " << Angle << "\n"
                   <<"\nStatistics\n"
                   <<"\tComplex verts: " << this->Stats[COMPLEX_VERTEX] << "\n"
                   <<"\tSimple verts: " << this->Stats[SIMPLE_VERTEX] << "\n"
                   <<"\tBoundary verts: " << this->Stats[BOUNDARY_VERTEX] << "\n"
                   <<"\tInterior edge verts: " << this->Stats[INTERIOR_EDGE_VERTEX] << "\n"
                   <<"\tCorner verts: " << this->Stats[CORNER_VERTEX] << "\n"
                   <<"\tEliminated via distance to plane: " << this->Stats[ELIMINATED_DISTANCE_TO_PLANE] << "\n"
                   <<"\tEliminated via distance to edge: " << this->Stats[ELIMINATED_DISTANCE_TO_EDGE] << "\n"
                   <<"\tFailed degree test: " << this->Stats[FAILED_DEGREE_TEST] << "\n"
                   <<"\tFailed non-manifold: " << this->Stats[FAILED_NON_MANIFOLD] << "\n"
                   <<"\tFailed zero area test: " << this->Stats[FAILED_ZERO_AREA_TEST] << "\n"
                   <<"\tFailed normal test: " << this->Stats[FAILED_ZERO_NORMAL_TEST] << "\n"
                   <<"\tFailed to triangulate: " << this->Stats[FAILED_TO_TRIANGULATE] << "\n");

      } //********************* sub iteration ******************************

    iteration++;
    criterion = this->Criterion +  iteration*this->CriterionIncrement;
    criterion = ((criterion > this->MaximumCriterion &&
    this->MaximumCriterion > 0.0) ? this->MaximumCriterion : criterion);
    Distance = max * criterion;
    Angle = this->FeatureAngle + iteration*this->FeatureAngleIncrement;
    Angle = ((Angle > this->MaximumFeatureAngle && 
              this->MaximumFeatureAngle > 0.0) ? this->MaximumFeatureAngle : Angle);
    CosAngle = cos ((double) math.DegreesToRadians() * Angle);

    } //********************************** outer loop **********************
//
//  Update output. This means renumbering points.
//
  vlDebugMacro (<<"Creating output...");

  map = new int[numPts];
  for (i=0; i<numPts; i++) map[i] = -1;
  numNewPts = 0;
  for (ptId=0; ptId < numPts; ptId++)
    {
    Mesh->GetPointCells(ptId,ncells,cells);
    if ( ncells > 0 ) map[ptId] = numNewPts++;
    }

  pd = this->Input->GetPointData();
  this->PointData.CopyAllocate(pd,numNewPts);
  newPts = new vlFloatPoints(numNewPts);

  for (ptId=0; ptId < numPts; ptId++)
    {
    if ( map[ptId] > -1 )
      {
      newPts->SetPoint(map[ptId],inPts->GetPoint(ptId));
      this->PointData.CopyData(pd,ptId,map[ptId]);
      }
    }

  // Now renumber connectivity
  newPolys = new vlCellArray;
  newPolys->Allocate(newPolys->EstimateSize(3,numTris-totalEliminated));

  for (cellId=0; cellId < numTris; cellId++)
    {
    if ( Mesh->GetCellType(cellId) == vlTRIANGLE ) // non-null element
      {
      Mesh->GetCellPoints(cellId, npts, pts);
      for (i=0; i<npts; i++) newCellPts[i] = map[pts[i]];
      newPolys->InsertNextCell(npts,newCellPts);
      }
    }

  delete [] map;
  delete Mesh; //side effect: releases memory consumned by data structures
  this->SetPoints(newPts);
  this->SetPolys(newPolys);
}


//
//  Build loop around vertex in question.  Basic intent of routine is
//  to identify the nature of the topolgy around the vertex.
//
int vlDecimate::BuildLoop (int ptId, unsigned short int numTris, int *tris)
{
  int numVerts;
  int numNei;
  static vlIdList nei(MAX_TRIS_PER_VERTEX);
  Tri t;
  Vertex sn;
  int i, j, *verts;
  int startVertex, nextVertex;
//
// Check complex cases
//
  if ( numTris >= this->Degree ) 
    {
    if ( Squawks++ < MAX_SQUAWKS ) 
      vlWarningMacro (<<"Exceeded maximum vertex degree");
    this->Stats[COMPLEX_VERTEX]++;
    this->Stats[FAILED_DEGREE_TEST]++;
    return COMPLEX_VERTEX;
    }
//
//  From the adjacency structure we can find the triangles that use the
//  vertex. Traverse this structure, gathering all the surrounding vertices
//  into an ordered list.
//
  V.Reset();
  T.Reset();

  sn.FAngle = 0.0;
  sn.deRefs =  2;  // keep track of triangle references to the verts 
  sn.newRefs = 0;

  t.n[0] = t.n[1] = t.n[2] = 0.0;
  t.verts[0] = -1; // Marks the fact that this poly hasn't been replaced 

//
//  Find the starting edge.  Do it very carefully do make sure
//  ordering is consistent (e.g., polygons ordering/normals remains consistent)
//
  Mesh->GetCellPoints(*tris,numVerts,verts); // get starting point
  for (i=0; i<3; i++) if (verts[i] == ptId) break;
  sn.id = startVertex = verts[(i+1)%3];

  V.InsertNextVertex(sn);

  nextVertex = -1; // initialize
  nei.Reset();
  nei.InsertId(0,*tris);
  numNei = 1;
//
//  Traverse the edge neighbors and see whether a cycle can be
//  completed.  Also have to keep track or orientation of faces for
//  computing normals.
//
  while ( T.MaxId < numTris && numNei == 1 && nextVertex != startVertex) 
    {
    t.id = nei.GetId(0);
    T.InsertNextTriangle(t);

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
    V.InsertNextVertex(sn);

    Mesh->GetCellEdgeNeighbors(t.id, ptId, nextVertex, nei);
    numNei = nei.GetNumberOfIds();
    } 
//
//  See whether we've run around the loop, hit a boundary, or hit a
//  complex spot.
//
  if ( nextVertex == startVertex && numNei == 1 ) 
    {
    if ( T.GetNumberOfTriangles() != numTris ) //touching non-manifold
      {
      this->Stats[FAILED_NON_MANIFOLD]++;
      this->Stats[COMPLEX_VERTEX]++;
      return COMPLEX_VERTEX;
      } 
    else  //remove last vertex addition
      {
      V.MaxId -= 1;
      this->Stats[SIMPLE_VERTEX]++;
      return SIMPLE_VERTEX;
      }
    }
//
//  Check for non-manifold cases
//
  else if ( numNei > 1 || T.GetNumberOfTriangles() > numTris ) 
    {
    if ( Squawks++ < MAX_SQUAWKS ) 
      vlWarningMacro(<<"Non-manifold geometry encountered");
    this->Stats[FAILED_NON_MANIFOLD]++;
    this->Stats[COMPLEX_VERTEX]++;
    return COMPLEX_VERTEX;
    }
//
//  Boundary loop - but (luckily) completed semi-cycle
//
  else if ( numNei == 0 && T.GetNumberOfTriangles() == numTris ) 
    {
    V.Array[0].FAngle = -1.0; // using cosine of -180 degrees
    V.Array[V.MaxId].FAngle = -1.0;
    V.Array[0].deRefs = 1;
    V.Array[V.MaxId].deRefs = 1;

    this->Stats[BOUNDARY_VERTEX]++;
    return BOUNDARY_VERTEX;
    }
//
//  Hit a boundary but didn't complete semi-cycle.  Gotta go back
//  around the other way.  Just reset the starting point and go 
//  back the other way.
//
  else 
    {
    t = T.GetTriangle(T.MaxId);

    V.Reset();
    T.Reset();

    startVertex = sn.id = nextVertex;
    V.InsertNextVertex(sn);

    nextVertex = -1;
    nei.Reset();
    nei.InsertId(0,t.id);
    numNei = 1;
//
//  Now move from boundary edge around the other way.
//
    while ( T.MaxId < numTris && numNei == 1 && nextVertex != startVertex) 
      {
      t.id = nei.GetId(0);
      T.InsertNextTriangle(t);

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
      V.InsertNextVertex(sn);

      Mesh->GetCellEdgeNeighbors(t.id, ptId, nextVertex, nei);
      numNei = nei.GetNumberOfIds();
      }
//
//  Make sure that there are only two boundaries (i.e., not non-manifold)
//
    if ( T.GetNumberOfTriangles() == numTris ) 
      {
//
//  Because we've reversed order of loop, need to rearrange the order
//  of the vertices and polygons to preserve consistent polygons
//  ordering / normal orientation.
//
      numVerts = V.GetNumberOfVertices();
      for (i=0; i<(numVerts/2); i++) 
        {
        sn.id = V.Array[i].id;
        V.Array[i].id = V.Array[numVerts-i-1].id;
        V.Array[numVerts-i-1].id = sn.id;
        }

      numTris = T.GetNumberOfTriangles();
      for (i=0; i<(numTris/2); i++) 
        {
        t.id = T.Array[i].id;
        T.Array[i].id = T.Array[numTris-i-1].id;
        T.Array[numTris-i-1].id = t.id;
        }

      V.Array[0].FAngle = -1.0;
      V.Array[V.MaxId].FAngle = -1.0;
      V.Array[0].deRefs = 1;
      V.Array[V.MaxId].deRefs = 1;

      this->Stats[BOUNDARY_VERTEX]++;
      return BOUNDARY_VERTEX;
      } 
    else // non-manifold
      {
      if ( Squawks++ < MAX_SQUAWKS ) 
        vlWarningMacro(<<"Non-manifold geometry encountered");
      this->Stats[FAILED_NON_MANIFOLD]++;
      this->Stats[COMPLEX_VERTEX]++;
      return COMPLEX_VERTEX;
      }
    }
}

#define FEATURE_ANGLE(tri1,tri2) math.Dot(T.Array[tri1].n, T.Array[tri2].n)

//
//  Compute the polygon normals and edge feature angles around the
//  loop.  Determine if there are any feature edges across the loop.
//
void vlDecimate::EvaluateLoop (int ptId, int& vtype, int& numFEdeges, 
                               VertexPtr fedges[])
{
  int i, j, numNormals;
  float *x1, *x2, *nx, *normal, den, loopArea;
  float v1[3], v2[3], center[3];
  int numFEdges;
//
//  Traverse all polygons and generate normals and areas
//
  nx = Mesh->GetPoint(ptId);
  x2 = Mesh->GetPoint(V.Array[0].id);
  for (i=0; i<3; i++) v2[i] = x2[i] - nx[i];

  loopArea=0.0;
  Normal[0] = Normal[1] = Normal[2] = 0.0;
  Pt[0] = Pt[1] = Pt[2] = 0.0;
  numNormals=0;

  for (i=0; i < T.GetNumberOfTriangles(); i++) 
    {
    normal = T.Array[i].n;
    x1 = x2;
    x2 = Mesh->GetPoint(V.Array[i+1].id);

    for (j=0; j<3; j++) 
      {
      v1[j] = v2[j];
      v2[j] = x2[j] - nx[j];
      }

    T.Array[i].area = triangle.TriangleArea (nx, x1, x2);
    triangle.TriangleCenter (nx, x1, x2, center);
    loopArea += T.Array[i].area;

    math.Cross (v1, v2, normal);
//
//  Get normals.  If null, then normal make no contribution to loop.
//  The center of the loop is the center of gravity.
//
    if ( (den=math.Norm(normal)) != 0.0 ) 
      {
      numNormals++;
      for (j=0; j<3; j++) 
        {
        normal[j] /= den;
        Normal[j] += T.Array[i].area * normal[j];
        Pt[j] += T.Array[i].area * center[j];
        }
      }
    }
//
//  Compute "average" plane normal and plane center.  Use an area
//  averaged normal calulation
//
  if ( !numNormals || loopArea == 0.0 ) 
    {
    this->Stats[FAILED_ZERO_AREA_TEST]++;
    vtype = COMPLEX_VERTEX;
    return;
    }

  for (j=0; j<3; j++) 
    {
    Normal[j] /= loopArea;
    Pt[j] /= loopArea;
    }
  if ( (den=math.Norm(Normal)) != 0.0 )
      for (j=0; j<3; j++)
          Normal[j] /= den;
  else 
    {
    this->Stats[FAILED_ZERO_NORMAL_TEST]++;
    vtype = COMPLEX_VERTEX;
    return;
    }
//
//  Now run through polygons again generating feature angles.  (Note
//  that if an edge is on the boundary its feature angle has already
//  been set to 180.)  Also need to keep track whether any feature
//  angles exceed the current value.
//
  if ( vtype == BOUNDARY_VERTEX ) 
    {
    numFEdges = 2;
    fedges[0] = V.Array;
    fedges[1] = V.Array + V.MaxId;
    } 
  else
    numFEdges = 0;
//
//  Compare to cosine of feature angle to avoid cosine extraction
//
  if ( vtype == SIMPLE_VERTEX ) // first edge 
    if ( (V.Array[0].FAngle = FEATURE_ANGLE(0,T.MaxId)) <= CosAngle )
      fedges[numFEdges++] = V.Array;

  for (i=0; i < T.MaxId; i++) 
    {
    if ( (V.Array[i+1].FAngle = FEATURE_ANGLE(i,i+1)) <= CosAngle ) 
      {
      if ( numFEdges >= 2 ) 
        numFEdges++;
      else 
        fedges[numFEdges] = V.Array + (i+1);
      }
    }
//
//  Final vtypeification
//
  if ( vtype == SIMPLE_VERTEX && numFEdges == 2 ) 
    {
    this->Stats[INTERIOR_EDGE_VERTEX]++;
    vtype = INTERIOR_EDGE_VERTEX;
    }
  else if ( vtype == SIMPLE_VERTEX && numFEdges > 0 ) 
    {
    this->Stats[CORNER_VERTEX]++;
    vtype = CORNER_VERTEX;
    }
}


//
//  Determine whether the loop can be split / build loops
//
int vlDecimate::CanSplitLoop (VertexPtr fedges[2], int numVerts, 
                              VertexPtr verts[], int& n1, VertexPtr l1[], 
                              int& n2, VertexPtr l2[], float& ar)
{
  int i, sign;
  float *x, val, absVal, sPt[3], v21[3], sN[3];
  float den, dist=LARGE_FLOAT;
//
//  See whether creating this edge would duplicate a new edge (this
//  means collapsing a tunnel)
//
  if ( Mesh->IsEdge(fedges[0]->id, fedges[1]->id) ) return 0;
//
//  Create two loops from the one using the splitting vertices provided.
//
  this->SplitLoop (fedges, numVerts, verts, n1, l1, n2, l2);
//
//  Create splitting plane.  Splitting plane is parallel to the loop
//  plane normal and contains the splitting vertices fedges[0] and fedges[1].
//
  Mesh->GetPoint(fedges[0]->id, sPt);
  Mesh->GetPoint(fedges[1]->id, v21);
  for (i=0; i<3; i++) v21[i] = v21[i] - sPt[i];

  math.Cross (v21,Normal,sN);
  if ( (den=math.Norm(sN)) != 0.0 )
    for (i=0; i<3; i++)
      sN[i] /= den;
  else
    return 0;
//
//  This plane can only be split if all points of each loop lie on the
//  same side of the splitting plane.  Also keep track of the minimum 
//  distance to the plane.
//
  for (sign=0, i=0; i < n1; i++) // first loop 
    {
    if ( !(l1[i] == fedges[0] || l1[i] == fedges[1]) ) 
      {
      x = Mesh->GetPoint(l1[i]->id);
      val = plane.Evaluate(sN,sPt,x);
      absVal = (float) fabs((double)val);
      dist = (absVal < dist ? absVal : dist);
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
      x = Mesh->GetPoint(l2[i]->id);
      val = plane.Evaluate(sN,sPt,x);
      absVal = (float) fabs((double)val);
      dist = (absVal < dist ? absVal : dist);
      if ( !sign )
        sign = (val > Tolerance ? 1 : -1);
      else if ( sign != (val > 0 ? 1 : -1) )
        return 0;
      }
    }
//
//  Now see if can split plane based on aspect ratio
//
  if ( (ar = (dist*dist)/(v21[0]*v21[0] + v21[1]*v21[1] + v21[2]*v21[2])) < AspectRatio2 )
    return 0;
  else
    return 1;
}

//
//  Creates two loops from splitting plane provided
//
void vlDecimate::SplitLoop(VertexPtr fedges[2], int numVerts, VertexPtr *verts,
                           int& n1, VertexPtr *l1, int& n2, VertexPtr *l2)
{
  int i;
  VertexPtr *loop;
  int *count;

  n1 = n2 = 0;
  loop = l1;
  count = &n1;

  for (i=0; i < numVerts; i++) 
    {
    loop[(*count)++] = verts[i];
    if ( verts[i] == fedges[0] || verts[i] == fedges[1] ) 
      {
      loop = (loop == l1 ? l2 : l1);
      count = (count == &n1 ? &n2 : &n1);
      loop[(*count)++] = verts[i];
      }
    }
}

//
//  Triangulate loop.  Use recursive divide and occur to reduce loop
//  into triangles.  Ignore feature angles since we can preserve these 
//  using the angle preserving capabilities of the algorithm.
//
void vlDecimate::Triangulate(int numVerts, VertexPtr verts[])
{
  int i,j;
  int n1, n2;
  VertexPtr l1[MAX_TRIS_PER_VERTEX], l2[MAX_TRIS_PER_VERTEX];
  VertexPtr fedges[2];
  float max, ar;
  int maxI, maxJ;

  if ( !ContinueTriangulating )
    return;

  switch (numVerts) 
    {
//
//  In loops of less than 3 vertices no elements are created
//
    case 0: case 1: case 2:
      return;
//
//  A loop of three vertices makes one triangle!  Replace an old
//  polygon with a newly created one.  
//
    case 3:
//
//  Make sure the new triangle doesn't duplicate an old one
//
      if ( Mesh->IsTriangle (verts[0]->id, verts[1]->id, verts[2]->id) ) 
        {
        ContinueTriangulating = 0;
        return;
        }
//
//  Okay: can create triangle, find a spot to put the triangle.
//
      for (i=0; i < T.MaxId; i++)
        if ( T.Array[i].verts[0] == -1 )
          break;

      for (j=0; j<3; j++) 
        {
        T.Array[i].verts[j] = verts[j]->id;
        verts[j]->newRefs++;
        }

      return;
//
//  Loops greater than three vertices must be subdivided.  This is
//  done by finding the best splitting plane and creating two loop and 
//  recursively triangulating.  To find the best splitting plane, try
//  all possible combinations, keeping track of the one that gives the
//  largest dihedral angle combination.
//
    default:
      max = 0.0;
      maxI = maxJ = -1;
      for (i=0; i<(numVerts-2); i++) 
        {
        for (j=i+2; j<numVerts; j++) 
          {
          if ( ((j+1) % numVerts) != i ) 
            {
            fedges[0] = verts[i];
            fedges[1] = verts[j];

            if ( this->CanSplitLoop(fedges, numVerts, verts, 
            n1, l1, n2, l2, ar) && ar > max ) 
              {
              max = ar;
              maxI = i;
              maxJ = j;
              }
            }
          }
        }

      if ( maxI > -1 ) 
        {
        fedges[0] = verts[maxI];
        fedges[1] = verts[maxJ];

        this->SplitLoop (fedges, numVerts, verts, n1, l1, n2, l2);

        this->Triangulate (n1, l1);
        this->Triangulate (n2, l2);

        return;
        }

      this->Stats[FAILED_TO_TRIANGULATE]++;
      ContinueTriangulating = 0;

      return;
    }
}

void vlDecimate::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlDecimate::GetClassName()))
    {
    vlPolyToPolyFilter::PrintSelf(os,indent);

    os << indent << "Target Reduction: " << this->TargetReduction << "\n";
    os << indent << "Criterion: " << this->Criterion << "\n";
    os << indent << "Criterion Increment: " << this->CriterionIncrement << "\n";
    os << indent << "Maximum Criterion: " << this->MaximumCriterion << "\n";
    os << indent << "Maximum Iterations: " << this->MaximumIterations << "\n";
    os << indent << "Maximum Sub Iterations: " << this->MaximumSubIterations << "\n";
    os << indent << "Aspect Ratio: " << this->AspectRatio << "\n";
    os << indent << "Preserve Edges: " << (this->PreserveEdges ? "On\n" : "Off\n");
    os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
    os << indent << "Feature Angle Increment: " << this->FeatureAngleIncrement << "\n";
    os << indent << "Maximum Feature Angle: " << this->MaximumFeatureAngle << "\n";

    }

}

