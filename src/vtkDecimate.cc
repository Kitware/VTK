/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecimate.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkDecimate.hh"

#define VTK_TOLERANCE 1.0e-05

#define VTK_MAX_TRIS_PER_VERTEX VTK_CELL_SIZE
#define VTK_MAX_SQUAWKS 10

#define VTK_COMPLEX_VERTEX 0
#define VTK_SIMPLE_VERTEX 1
#define VTK_BOUNDARY_VERTEX 2
#define VTK_INTERIOR_EDGE_VERTEX 3
#define VTK_CORNER_VERTEX 4

#define VTK_ELIMINATED_DISTANCE_TO_PLANE 5
#define VTK_ELIMINATED_DISTANCE_TO_EDGE 6
#define VTK_FAILED_DEGREE_TEST 7
#define VTK_FAILED_NON_MANIFOLD 8
#define VTK_FAILED_ZERO_AREA_TEST 9
#define VTK_FAILED_ZERO_NORMAL_TEST 10
#define VTK_FAILED_TO_TRIANGULATE 11

//
//  Static variables used by object
//
static vtkPlane plane; // eliminate constructor overhead
static vtkLine line;
static vtkTriangle triangle;
static vtkMath math;

static vtkPolyData *Mesh; // operate on this data structure
static float Pt[3], Normal[3]; // least squares plane point & normal
static float Angle, Distance; // current feature angle and distance 
static float CosAngle; // Cosine of dihedral angle

static float Tolerance; // Intersection tolerance
static float AspectRatio2; // Allowable aspect ratio 
static int ContinueTriangulating; // Stops recursive tri. if necessary 
static int Squawks; // Control output 

static float X[3]; //coordinates of current point
static float *VertexError, Error, MinEdgeError; //support error omputation

static vtkVertexArray *V; //cycle of vertices around point
static vtkTriArray *T; //cycle of triangles around point


// Description:
// Create object with target reduction of 90%, feature angle of 30 degrees, 
// initial error of 0.0, error increment of 0.005, maximum error of 0.1, and
// maximum iterations of 6.
vtkDecimate::vtkDecimate()
{
  this->InitialFeatureAngle = 30;
  this->FeatureAngleIncrement = 0.0;
  this->MaximumFeatureAngle = 60;
  this->PreserveEdges = 1;
  this->BoundaryVertexDeletion = 1;

  this->InitialError = 0.0;
  this->ErrorIncrement = 0.005;
  this->MaximumError = 0.1;

  this->TargetReduction = 0.90;

  this->MaximumIterations = 6;
  this->MaximumSubIterations = 2;

  this->AspectRatio = 25.0;

  this->Degree = 25;

  this->GenerateErrorScalars = 0;
}

//
//  Reduce triangles in mesh by given amount or until total number of
//  iterations completes
//
void vtkDecimate::Execute()
{
  int numPts, numTris = 0;
  vtkPoints *inPts;
  vtkCellArray *inPolys;
  int numVerts = 0;
  float *bounds, max;
  int i, ptId;
  vtkCellArray *newPolys;
  float reduction=0.0;
  int iteration=0, sub;
  int trisEliminated;
  unsigned short int ncells;
  int *cells;
  int numFEdges;
  vtkLocalVertexPtr fedges[2];
  int vtype;
  vtkLocalVertexPtr verts[VTK_MAX_TRIS_PER_VERTEX];
  vtkLocalVertexPtr l1[VTK_MAX_TRIS_PER_VERTEX], l2[VTK_MAX_TRIS_PER_VERTEX];
  int n1, n2;
  float ar, error;
  int totalEliminated=0;
  int size;
  vtkPolyData *input=(vtkPolyData *)this->Input;

  // do it this way because some compilers can't handle construction of
  // static objects in file scope.
  static vtkVertexArray VertexArray(VTK_MAX_TRIS_PER_VERTEX+1);
  static vtkTriArray TriangleArray(VTK_MAX_TRIS_PER_VERTEX+1);

  vtkDebugMacro(<<"Decimating mesh...");
  V = &VertexArray;
  T = &TriangleArray;
//
// Check input
//
  if ( (numPts=input->GetNumberOfPoints()) < 1 || 
  (numTris=input->GetNumberOfPolys()) < 1 )
    {
    vtkErrorMacro(<<"No data to decimate!");
    return;
    }
//
//  Get the bounds of the data to compute decimation threshold
//
  bounds = input->GetBounds();

  for (max=0.0, i=0; i<3; i++)
    max = ((bounds[2*i+1]-bounds[2*i]) > max ? 
           (bounds[2*i+1]-bounds[2*i]) : max);

  Tolerance = max * VTK_TOLERANCE;
  error = this->InitialError;
  Distance = error * max;
  Angle = this->InitialFeatureAngle;
  CosAngle = cos ((double) math.DegreesToRadians() * Angle);
  AspectRatio2 = 1.0 / (this->AspectRatio * this->AspectRatio);
  Squawks = 0;

  vtkDebugMacro(<<"Decimating " << numPts << " vertices, " << numTris 
               << " triangles with:\n"
               << "\tIterations= " << this->MaximumIterations << "\n"
               << "\tSub-iterations= " << this->MaximumSubIterations << "\n"
               << "\tLength= " << max << "\n"
               << "\tError= " << this->InitialError << "\n"
               << "\tDistance= " << Distance << "\n"
               << "\tAspect ratio= " << this->AspectRatio << "\n"
               << "\tMaximum vertex degree= " << this->Degree);
//
// Build cell data structure. Need to copy triangle connectivity data
// so we can modify it.
//
  inPts = input->GetPoints();
  inPolys = input->GetPolys();
  Mesh = new vtkPolyData;
  Mesh->SetPoints(inPts);
  newPolys = new vtkCellArray(*(inPolys));
  Mesh->SetPolys(newPolys);
  newPolys->Delete(); //registered by Mesh and preserved
  Mesh->BuildLinks();
//
// Create array of vertex errors (initially zero)
//
  VertexError = new float[numPts];
  for (i=0; i<numPts; i++) VertexError[i] = 0.0;
//
//  Traverse all vertices, eliminating those that meet decimation
//  error.  Initialize loop information.
//
//************************************ Outer Loop ***************************

  while ( reduction < this->TargetReduction && iteration < this->MaximumIterations ) 
    {
    trisEliminated = 1;

//******************************** Subiterations ****************************

    for (sub=0; sub < this->MaximumSubIterations && trisEliminated &&
    reduction < this->TargetReduction; sub++) 
      {
      for (i=0; i < VTK_NUMBER_STATISTICS; i++) this->Stats[i] = 0;
      trisEliminated = 0;
//
//  For every vertex that is used by two or more elements and has a loop
//  of simple enough complexity...
//
      for (ptId=0; ptId < numPts; ptId++)
        {
        if ( ! (ptId % 5000) ) vtkDebugMacro(<<"vertex #" << ptId);

        // compute allowable error for this vertex
        Mesh->GetPoint(ptId,X);
        Error = Distance - VertexError[ptId];
        MinEdgeError = VTK_LARGE_FLOAT;

        Mesh->GetPointCells(ptId,ncells,cells);
        if ( ncells > 1 && 
        (vtype=this->BuildLoop(ptId,ncells,cells)) != VTK_COMPLEX_VERTEX )
          {
//
//  Determine the distance of the vertex to an "average plane"
//  through the loop.  If it's less than the decimation distance
//  criterion, then vertex can be eliminated.  If the vertex is on the
//  boundary, see whether it can be eliminated based on distance to
//  boundary.
//
          ContinueTriangulating = 0;
          this->EvaluateLoop (vtype, numFEdges, fedges);

          if ( vtype != VTK_COMPLEX_VERTEX ) 
            {
            ContinueTriangulating = 1;
            numVerts = V->GetNumberOfVertices();
            for (i=0; i < numVerts; i++) verts[i] = V->Array + i;
            }
//
//  Note: interior edges can be eliminated if decimation criterion met
//  and flag set.
//
          if ( (vtype == VTK_SIMPLE_VERTEX || 
          ( (vtype == VTK_INTERIOR_EDGE_VERTEX || vtype == VTK_CORNER_VERTEX) && !this->PreserveEdges)) &&
          plane.DistanceToPlane(X,Normal,Pt) <= Error)
            {
            this->Triangulate (numVerts, verts);
            this->Stats[VTK_ELIMINATED_DISTANCE_TO_PLANE]++;
//
//  If the vertex is on an interior edge, then see whether the
//  be eliminated based on distance to line (e.g., edge), and it the
//  loop can be split.  
//
            } 
          else if ((vtype == VTK_INTERIOR_EDGE_VERTEX || 
          vtype == VTK_BOUNDARY_VERTEX) && this->BoundaryVertexDeletion &&
          line.DistanceToLine(X,fedges[0]->x,fedges[1]->x) <= (Error*Error) &&
          this->CanSplitLoop (fedges,numVerts,verts,n1,l1,n2,l2,ar) )
            {
            this->Triangulate (n1, l1);
            this->Triangulate (n2, l2);
            this->Stats[VTK_ELIMINATED_DISTANCE_TO_EDGE]++;
            } 
          else
            {
            ContinueTriangulating = 0;
            }

          if ( ContinueTriangulating ) 
            {
            if ( this->CheckError() )
              {
              if ( vtype == VTK_BOUNDARY_VERTEX ) trisEliminated += 1;
              else trisEliminated += 2;

              // Update the data structure to reflect deletion of vertex
              Mesh->DeletePoint(ptId);
              for (i=0; i < V->GetNumberOfVertices(); i++)
                if ( (size=V->Array[i].newRefs-V->Array[i].deRefs) > 0 )
                  Mesh->ResizeCellList(V->Array[i].id,size);

              for (i=0; i < T->GetNumberOfTriangles(); i++)
                Mesh->RemoveCellReference(T->Array[i].id);

              for (i=0; i < T->GetNumberOfTriangles(); i++)
                if ( T->Array[i].verts[0] != -1 ) //replaced with new triangle
                  Mesh->ReplaceLinkedCell(T->Array[i].id, 3, T->Array[i].verts);
                else
                  Mesh->DeleteCell(T->Array[i].id);

              }
            }
          }
        }

      totalEliminated += trisEliminated;
      reduction = (float) totalEliminated / numTris;

      vtkDebugMacro(<<"\n\tIteration = " << iteration+1 << "\n"
                   <<"\tSub-iteration = " << sub+1 << "\n"
                   <<"\tPolygons removed = " << trisEliminated << "\n"
                   <<"\tTotal removed = " << totalEliminated << "\n"
                   <<"\tRemaining = " << numTris - totalEliminated << "\n"
                   <<"\tOriginal triangles = " << numTris << "\n"
                   <<"\tReduction = " << reduction << "\n"
                   <<"\tError = " << error << "\n"
                   <<"\tDistance = " << Distance << "\n"
                   <<"\tFeature angle = " << Angle << "\n"
                   <<"\nStatistics\n"
                   <<"\tComplex verts: " << this->Stats[VTK_COMPLEX_VERTEX] << "\n"
                   <<"\tSimple verts: " << this->Stats[VTK_SIMPLE_VERTEX] << "\n"
                   <<"\tBoundary verts: " << this->Stats[VTK_BOUNDARY_VERTEX] << "\n"
                   <<"\tInterior edge verts: " << this->Stats[VTK_INTERIOR_EDGE_VERTEX] << "\n"
                   <<"\tCorner verts: " << this->Stats[VTK_CORNER_VERTEX] << "\n"
                   <<"\tEliminated via distance to plane: " << this->Stats[VTK_ELIMINATED_DISTANCE_TO_PLANE] << "\n"
                   <<"\tEliminated via distance to edge: " << this->Stats[VTK_ELIMINATED_DISTANCE_TO_EDGE] << "\n"
                   <<"\tFailed degree test: " << this->Stats[VTK_FAILED_DEGREE_TEST] << "\n"
                   <<"\tFailed non-manifold: " << this->Stats[VTK_FAILED_NON_MANIFOLD] << "\n"
                   <<"\tFailed zero area test: " << this->Stats[VTK_FAILED_ZERO_AREA_TEST] << "\n"
                   <<"\tFailed normal test: " << this->Stats[VTK_FAILED_ZERO_NORMAL_TEST] << "\n"
                   <<"\tFailed to triangulate: " << this->Stats[VTK_FAILED_TO_TRIANGULATE] << "\n");

      } //********************* sub iteration ******************************

    iteration++;
    error = this->InitialError +  iteration*this->ErrorIncrement;
    error = ((error > this->MaximumError &&
    this->MaximumError > 0.0) ? this->MaximumError : error);
    Distance = max * error;
    Angle = this->InitialFeatureAngle + iteration*this->FeatureAngleIncrement;
    Angle = ((Angle > this->MaximumFeatureAngle && 
              this->MaximumFeatureAngle > 0.0) ? this->MaximumFeatureAngle : Angle);
    CosAngle = cos ((double) math.DegreesToRadians() * Angle);

    } //********************************** outer loop **********************
//
//  Update output. This means renumbering points.
//
  this->CreateOutput(numPts, numTris, totalEliminated, input->GetPointData(), inPts);
}

void vtkDecimate::CreateOutput(int numPts, int numTris, int numEliminated,
                              vtkPointData *pd, vtkPoints *inPts)
{
  int *map, numNewPts;
  int i;
  int newCellPts[VTK_CELL_SIZE];
  unsigned short int ncells;
  int *cells;
  int ptId, cellId, npts, *pts;
  vtkFloatPoints *newPts;
  vtkCellArray *newPolys;
  vtkFloatScalars *newScalars = NULL;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  vtkDebugMacro (<<"Creating output...");

  if ( ! this->GenerateErrorScalars )
    delete [] VertexError;

  map = new int[numPts];
  for (i=0; i<numPts; i++) map[i] = -1;
  numNewPts = 0;
  for (ptId=0; ptId < numPts; ptId++)
    {
    Mesh->GetPointCells(ptId,ncells,cells);
    if ( ncells > 0 ) map[ptId] = numNewPts++;
    }

  if ( this->GenerateErrorScalars ) 
    {
    outputPD->CopyScalarsOff();
    }
  outputPD->CopyAllocate(pd,numNewPts);
  newPts = new vtkFloatPoints(numNewPts);

  for (ptId=0; ptId < numPts; ptId++)
    {
    if ( map[ptId] > -1 )
      {
      newPts->SetPoint(map[ptId],inPts->GetPoint(ptId));
      outputPD->CopyData(pd,ptId,map[ptId]);
      }
    }

  if ( this->GenerateErrorScalars )
    {
    newScalars = new vtkFloatScalars[numNewPts];
    for (ptId=0; ptId < numPts; ptId++)
      if ( map[ptId] > -1 )
        newScalars->SetScalar(map[ptId],VertexError[ptId]);
    }

  // Now renumber connectivity
  newPolys = new vtkCellArray;
  newPolys->Allocate(newPolys->EstimateSize(3,numTris-numEliminated));

  for (cellId=0; cellId < numTris; cellId++)
    {
    if ( Mesh->GetCellType(cellId) == VTK_TRIANGLE ) // non-null element
      {
      Mesh->GetCellPoints(cellId, npts, pts);
      for (i=0; i<npts; i++) newCellPts[i] = map[pts[i]];
      newPolys->InsertNextCell(npts,newCellPts);
      }
    }

  delete [] map;
  Mesh->Delete();
  output->SetPoints(newPts);
  output->SetPolys(newPolys);

  newPts->Delete();
  newPolys->Delete();
  if ( this->GenerateErrorScalars )
    {
    outputPD->SetScalars(newScalars);
    newScalars->Delete();
    delete [] VertexError;
    }
}

//
//  Build loop around vertex in question.  Basic intent of routine is
//  to identify the nature of the topolgy around the vertex.
//
int vtkDecimate::BuildLoop (int ptId, unsigned short int numTris, int *tris)
{
  int numVerts;
  int numNei;
  static vtkIdList nei(VTK_MAX_TRIS_PER_VERTEX);
  vtkLocalTri t;
  vtkLocalVertex sn;
  int i, j, *verts;
  int startVertex, nextVertex;
//
// Check complex cases
//
  if ( numTris >= this->Degree ) 
    {
    if ( Squawks++ < VTK_MAX_SQUAWKS ) 
      vtkWarningMacro (<<"Exceeded maximum vertex degree");
    this->Stats[VTK_COMPLEX_VERTEX]++;
    this->Stats[VTK_FAILED_DEGREE_TEST]++;
    return VTK_COMPLEX_VERTEX;
    }
//
//  From the adjacency structure we can find the triangles that use the
//  vertex. Traverse this structure, gathering all the surrounding vertices
//  into an ordered list.
//
  V->Reset();
  T->Reset();

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
  Mesh->GetPoint(sn.id, sn.x); //grab coordinates here to save GetPoint() calls

  V->InsertNextVertex(sn);

  nextVertex = -1; // initialize
  nei.Reset();
  nei.InsertId(0,*tris);
  numNei = 1;
//
//  Traverse the edge neighbors and see whether a cycle can be
//  completed.  Also have to keep track or orientation of faces for
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
      this->Stats[VTK_FAILED_NON_MANIFOLD]++;
      this->Stats[VTK_COMPLEX_VERTEX]++;
      return VTK_COMPLEX_VERTEX;
      } 
    else  //remove last vertex addition
      {
      V->MaxId -= 1;
      this->Stats[VTK_SIMPLE_VERTEX]++;
      return VTK_SIMPLE_VERTEX;
      }
    }
//
//  Check for non-manifold cases
//
  else if ( numNei > 1 || T->GetNumberOfTriangles() > numTris ) 
    {
    if ( Squawks++ < VTK_MAX_SQUAWKS ) 
      vtkWarningMacro(<<"Non-manifold geometry encountered");
    this->Stats[VTK_FAILED_NON_MANIFOLD]++;
    this->Stats[VTK_COMPLEX_VERTEX]++;
    return VTK_COMPLEX_VERTEX;
    }
//
//  Boundary loop - but (luckily) completed semi-cycle
//
  else if ( numNei == 0 && T->GetNumberOfTriangles() == numTris ) 
    {
    V->Array[0].FAngle = -1.0; // using cosine of -180 degrees
    V->Array[V->MaxId].FAngle = -1.0;
    V->Array[0].deRefs = 1;
    V->Array[V->MaxId].deRefs = 1;

    this->Stats[VTK_BOUNDARY_VERTEX]++;
    return VTK_BOUNDARY_VERTEX;
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
      V->Array[0].deRefs = 1;
      V->Array[V->MaxId].deRefs = 1;

      this->Stats[VTK_BOUNDARY_VERTEX]++;
      return VTK_BOUNDARY_VERTEX;
      } 
    else // non-manifold
      {
      if ( Squawks++ < VTK_MAX_SQUAWKS ) 
        vtkWarningMacro(<<"Non-manifold geometry encountered");
      this->Stats[VTK_FAILED_NON_MANIFOLD]++;
      this->Stats[VTK_COMPLEX_VERTEX]++;
      return VTK_COMPLEX_VERTEX;
      }
    }
}

#define FEATURE_ANGLE(tri1,tri2) math.Dot(T->Array[tri1].n, T->Array[tri2].n)

//
//  Compute the polygon normals and edge feature angles around the
//  loop.  Determine if there are any feature edges across the loop.
//
void vtkDecimate::EvaluateLoop (int& vtype, int& numFEdges, 
                               vtkLocalVertexPtr fedges[])
{
  int i, j, numNormals;
  float *x1, *x2, *normal, loopArea;
  float v1[3], v2[3], center[3];
//
//  Traverse all polygons and generate normals and areas
//
  x2 =  V->Array[0].x;
  for (i=0; i<3; i++) v2[i] = x2[i] - X[i];

  loopArea=0.0;
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

    T->Array[i].area = triangle.TriangleArea (X, x1, x2);
    triangle.TriangleCenter (X, x1, x2, center);
    loopArea += T->Array[i].area;

    math.Cross (v1, v2, normal);
//
//  Get normals.  If null, then normal make no contribution to loop.
//  The center of the loop is the center of gravity.
//
    if ( math.Normalize(normal) != 0.0 ) 
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
  if ( !numNormals || loopArea == 0.0 ) 
    {
    this->Stats[VTK_FAILED_ZERO_AREA_TEST]++;
    vtype = VTK_COMPLEX_VERTEX;
    return;
    }

  for (j=0; j<3; j++) 
    {
    Normal[j] /= loopArea;
    Pt[j] /= loopArea;
    }
  if ( math.Normalize(Normal) == 0.0 )
    {
    this->Stats[VTK_FAILED_ZERO_NORMAL_TEST]++;
    vtype = VTK_COMPLEX_VERTEX;
    return;
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
    fedges[0] = V->Array;
    fedges[1] = V->Array + V->MaxId;
    } 
  else
    numFEdges = 0;
//
//  Compare to cosine of feature angle to avoid cosine extraction
//
  if ( vtype == VTK_SIMPLE_VERTEX ) // first edge 
    if ( (V->Array[0].FAngle = FEATURE_ANGLE(0,T->MaxId)) <= CosAngle )
      fedges[numFEdges++] = V->Array;

  for (i=0; i < T->MaxId; i++) 
    {
    if ( (V->Array[i+1].FAngle = FEATURE_ANGLE(i,i+1)) <= CosAngle ) 
      {
      if ( numFEdges >= 2 ) 
        numFEdges++;
      else 
        fedges[numFEdges++] = V->Array + (i+1);
      }
    }
//
//  Final classification
//
  if ( vtype == VTK_SIMPLE_VERTEX && numFEdges == 2 ) 
    {
    this->Stats[VTK_INTERIOR_EDGE_VERTEX]++;
    vtype = VTK_INTERIOR_EDGE_VERTEX;
    }
  else if ( vtype == VTK_SIMPLE_VERTEX && numFEdges > 0 ) 
    {
    this->Stats[VTK_CORNER_VERTEX]++;
    vtype = VTK_CORNER_VERTEX;
    }
}


//
//  Determine whether the loop can be split / build loops
//
int vtkDecimate::CanSplitLoop (vtkLocalVertexPtr fedges[2], int numVerts, 
                              vtkLocalVertexPtr verts[], int& n1, 
                              vtkLocalVertexPtr l1[], int& n2, 
                              vtkLocalVertexPtr l2[], float& ar)
{
  int i, sign;
  float *x, val, absVal, sPt[3], v21[3], sN[3];
  float dist=VTK_LARGE_FLOAT;
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
  for (i=0; i<3; i++) 
    {
    sPt[i] = fedges[0]->x[i];
    v21[i] = fedges[1]->x[i] - sPt[i];
    }

  math.Cross (v21,Normal,sN);
  if ( math.Normalize(sN) == 0.0 ) return 0;
//
//  This plane can only be split if all points of each loop lie on the
//  same side of the splitting plane.  Also keep track of the minimum 
//  distance to the plane.
//
  for (sign=0, i=0; i < n1; i++) // first loop 
    {
    if ( !(l1[i] == fedges[0] || l1[i] == fedges[1]) ) 
      {
      x = l1[i]->x;
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
      x = l2[i]->x;
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
void vtkDecimate::SplitLoop(vtkLocalVertexPtr fedges[2], int numVerts, 
                           vtkLocalVertexPtr *verts, int& n1, 
                           vtkLocalVertexPtr *l1, int& n2, vtkLocalVertexPtr *l2)
{
  int i;
  vtkLocalVertexPtr *loop;
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
void vtkDecimate::Triangulate(int numVerts, vtkLocalVertexPtr verts[])
{
  int i,j;
  int n1, n2;
  vtkLocalVertexPtr l1[VTK_MAX_TRIS_PER_VERTEX], l2[VTK_MAX_TRIS_PER_VERTEX];
  vtkLocalVertexPtr fedges[2];
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
      for (i=0; i < T->MaxId; i++)
        if ( T->Array[i].verts[0] == -1 )
          break;

      for (j=0; j<3; j++) 
        {
        T->Array[i].verts[j] = verts[j]->id;
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
        float edgeError;

        fedges[0] = verts[maxI];
        fedges[1] = verts[maxJ];

        this->SplitLoop (fedges, numVerts, verts, n1, l1, n2, l2);

        this->Triangulate (n1, l1);
        this->Triangulate (n2, l2);

        // Compute minimum edge error
        edgeError = line.DistanceToLine(X, fedges[0]->x, fedges[1]->x);

        if ( edgeError < MinEdgeError ) MinEdgeError = edgeError;

        return;
        }

      this->Stats[VTK_FAILED_TO_TRIANGULATE]++;
      ContinueTriangulating = 0;

      return;
    }
}

int vtkDecimate::CheckError ()
{
    int i, j;
    float error, planeError;
    float normal[3], np[3], v21[3], v31[3], *x1, *x2, *x3;
//
//  Loop through triangles computing distance to plane (looking for minimum
//  perpendicular distance)
//
  for (planeError=VTK_LARGE_FLOAT, i=0; i < T->GetNumberOfTriangles(); i++) 
    {
    if ( T->Array[i].verts[0] == -1 ) break;

    x1 = Mesh->GetPoint(T->Array[i].verts[0]);
    x2 = Mesh->GetPoint(T->Array[i].verts[1]);
    x3 = Mesh->GetPoint(T->Array[i].verts[2]);

    for (j=0; j<3; j++) 
      {
      v21[j] = x2[j] - x1[j];
      v31[j] = x3[j] - x1[j];
      }

    math.Cross (v31, v21, normal);

    if ( math.Normalize(normal) != 0.0 ) 
      {
      for (j=0; j<3; j++) np[j] = X[j] - x1[j];
      error = fabs((double) (math.Dot(normal,np)));
      if ( error < planeError ) planeError = error;
      }
    }

  if ( MinEdgeError > 0.0 ) 
    MinEdgeError = sqrt((double)MinEdgeError);
  else 
    MinEdgeError = 0.0;

  error = (planeError < MinEdgeError ? planeError : MinEdgeError);    
  if ( error > Error ) return 0;
//
// Can distribute errors to surrounding nodes
//
  for (i=0; i < V->GetNumberOfVertices(); i++)
    VertexError[V->Array[i].id] += error;

  return 1; // okay to delete; error computed and distributed
}

void vtkDecimate::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Target Reduction: " << this->TargetReduction << "\n";
  os << indent << "Initial Error: " << this->InitialError << "\n";
  os << indent << "Error Increment: " << this->ErrorIncrement << "\n";
  os << indent << "Maximum Error: " << this->MaximumError << "\n";
  os << indent << "Maximum Iterations: " << this->MaximumIterations << "\n";
  os << indent << "Maximum Sub Iterations: " << this->MaximumSubIterations << "\n";
  os << indent << "Aspect Ratio: " << this->AspectRatio << "\n";
  os << indent << "Preserve Edges: " << (this->PreserveEdges ? "On\n" : "Off\n");
  os << indent << "Initial Feature Angle: " << this->InitialFeatureAngle << "\n";
  os << indent << "Feature Angle Increment: " << this->FeatureAngleIncrement << "\n";
  os << indent << "Maximum Feature Angle: " << this->MaximumFeatureAngle << "\n";
  os << indent << "Generate Error Scalars: " << (this->GenerateErrorScalars ? "On\n" : "Off\n");


}

