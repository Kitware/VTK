/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecimatePro.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDecimatePro.h"

#include "vtkDoubleArray.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPriorityQueue.h"
#include "vtkTriangle.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

vtkStandardNewMacro(vtkDecimatePro);

#define VTK_TOLERANCE 1.0e-05
#define VTK_MAX_TRIS_PER_VERTEX VTK_CELL_SIZE
#define VTK_RECYCLE_VERTEX VTK_DOUBLE_MAX

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

// Helper functions
static double ComputeSimpleError(double x[3], double normal[3], double point[3]);
static double ComputeEdgeError(double x[3], double x1[3], double x2[3]);
static double ComputeSingleTriangleError(double x[3], double x1[3], double x2[3]);


// Create object with specified reduction of 90% and feature angle of
// 15 degrees. Edge splitting is on, defer splitting is on, and the
// split angle is 75 degrees. Topology preservation is off, delete
// boundary vertices is on, and the maximum error is set to
// VTK_DOUBLE_MAX. The inflection point ratio is 10 and the vertex
// degree is 25. Error accumulation is turned off.
vtkDecimatePro::vtkDecimatePro()
{
  this->Neighbors = vtkIdList::New();
  this->Neighbors->Allocate(VTK_MAX_TRIS_PER_VERTEX);
  this->V = new vtkDecimatePro::VertexArray(VTK_MAX_TRIS_PER_VERTEX+1);
  this->T = new vtkDecimatePro::TriArray(VTK_MAX_TRIS_PER_VERTEX+1);
  this->EdgeLengths = vtkPriorityQueue::New();
  this->EdgeLengths->Allocate(VTK_MAX_TRIS_PER_VERTEX);
  
  this->InflectionPoints = vtkDoubleArray::New();
  this->TargetReduction = 0.90;
  this->FeatureAngle = 15.0;
  this->PreserveTopology = 0;
  this->MaximumError = VTK_DOUBLE_MAX;
  this->AbsoluteError = VTK_DOUBLE_MAX;
  this->ErrorIsAbsolute = 0;
  this->AccumulateError = 0;
  this->SplitAngle = 75.0;
  this->Splitting = 1;
  this->PreSplitMesh = 0;
  this->Degree = 25;
  this->BoundaryVertexDeletion = 1;
  this->InflectionPointRatio = 10.0;

  this->Queue = NULL;
  this->VertexError = NULL;

  this->Mesh = NULL;
}

vtkDecimatePro::~vtkDecimatePro()
{
  this->InflectionPoints->Delete();
  if ( this->Queue )
    {
    this->Queue->Delete();
    }
  if ( this->VertexError )
    {
    this->VertexError->Delete();
    }
  this->Neighbors->Delete();
  this->EdgeLengths->Delete();
  delete this->V;
  delete this->T;
}

//
//  Reduce triangles in mesh by specified reduction factor.
//
int vtkDecimatePro::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType i, ptId, numPts, numTris, collapseId;
  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkCellArray *inPolys;
  vtkCellArray *newPolys;
  double error, previousError=0.0, reduction;
  int type;
  vtkIdType *pts, npts, totalEliminated, numRecycles, numPops;
  unsigned short int ncells;
  vtkIdType pt1, pt2, cellId, fedges[2];
  vtkIdType *cells;
  vtkIdList *CollapseTris;
  double max, *bounds;
  if (!input)
    {
    vtkErrorMacro(<<"No input!");
    return 1;
    }
  vtkPointData *outputPD=output->GetPointData();
  vtkPointData *inPD=input->GetPointData();
  vtkPointData *meshPD=0;
  vtkIdType *map, numNewPts, totalPts;
  vtkIdType newCellPts[3];
  int abortExecute=0;

  vtkDebugMacro(<<"Executing progressive decimation...");

  // Check input
  this->NumberOfRemainingTris = numTris = input->GetNumberOfPolys();
  if ( ((numPts=input->GetNumberOfPoints()) < 1 || numTris < 1) &&
       (this->TargetReduction > 0.0) )
    {
    vtkErrorMacro(<<"No data to decimate!");
    return 1;
    }
  
  // Initialize
  bounds = input->GetBounds();
  for (max=0.0, i=0; i<3; i++)
    {
    max = ((bounds[2*i+1]-bounds[2*i]) > max ? 
           (bounds[2*i+1]-bounds[2*i]) : max);
    }
  if (!this->ErrorIsAbsolute)
    {
    this->Error = (this->MaximumError >= VTK_DOUBLE_MAX ?
                   VTK_DOUBLE_MAX : this->MaximumError * max);
    }
  else
    {
    this->Error = (this->AbsoluteError >= VTK_DOUBLE_MAX ?
                   VTK_DOUBLE_MAX : this->AbsoluteError);
    }
  this->Tolerance = VTK_TOLERANCE * input->GetLength();
  this->CosAngle = cos( vtkMath::RadiansFromDegrees( this->FeatureAngle) );
  this->Split = ( this->Splitting && !this->PreserveTopology );
  this->VertexDegree = this->Degree;
  this->TheSplitAngle = this->SplitAngle;
  this->SplitState = VTK_STATE_UNSPLIT;

  // Lets check to make sure there are only triangles in the input.
  vtkIdType *pPolys;
  pPolys = input->GetPolys()->GetPointer();
  for (i = 0; i < numTris; ++i)
    {
    if (*pPolys != 3)
      {
      vtkErrorMacro("DecimatePro does not accept polygons that are not triangles.");
      output->CopyStructure(input);
      output->GetPointData()->PassData(input->GetPointData());
      output->GetCellData()->PassData(input->GetCellData());
      return 1;
      }
    pPolys += 4;
    }

  // Build cell data structure. Need to copy triangle connectivity data
  // so we can modify it.
  if ( this->TargetReduction > 0.0 )
    {
    inPts = input->GetPoints();
    inPolys = input->GetPolys();

    // this static should be eliminated
    if (this->Mesh != NULL) {this->Mesh->Delete(); this->Mesh = NULL;}
    this->Mesh = vtkPolyData::New();
    
    newPts = vtkPoints::New(); newPts->SetNumberOfPoints(numPts);
    newPts->DeepCopy(inPts);
    this->Mesh->SetPoints(newPts);
    newPts->Delete(); //registered by Mesh and preserved
    
    newPolys = vtkCellArray::New();
    newPolys->DeepCopy(inPolys);
    this->Mesh->SetPolys(newPolys);
    newPolys->Delete(); //registered by Mesh and preserved
    
    meshPD = this->Mesh->GetPointData();
    meshPD->DeepCopy(inPD);
    meshPD->CopyAllocate(meshPD, input->GetNumberOfPoints());
    
    this->Mesh->BuildLinks();
    }
  else
    {
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    //vtkWarningMacro(<<"Reduction == 0: passing data through unchanged");
    return 1;
    }
  
  // Initialize data structures: priority queue and errors.
  this->InitializeQueue(numPts);
  
  if ( this->AccumulateError )
    {
    this->VertexError = vtkDoubleArray::New();
    this->VertexError->Allocate(numPts,static_cast<vtkIdType>(0.25*numPts));
    for (i=0; i<numPts; i++)
      {
      this->VertexError->SetValue(i, 0.0);
      }
    }
  
  // If not deferring splitting and splitting on, we'll start off by 
  // splitting the mesh. This has side effect of inserting vertices.
  this->NumCollapses = this->NumMerges = 0;
  if ( this->Split && this->PreSplitMesh )
    {
    vtkDebugMacro(<<"Pre-splitting mesh");
    this->SplitState = VTK_STATE_SPLIT;
    this->SplitMesh();
    }
  
  // Start by traversing all vertices. For each vertex, evaluate the
  // local topology/geometry. (Some vertex splitting may be
  // necessary to resolve non-manifold geometry or to split edges.) 
  // Then evaluate the local error for the vertex. The vertex is then
  // inserted into the priority queue.
  npts = this->Mesh->GetNumberOfPoints();
  for ( ptId=0; ptId < npts && !abortExecute ; ptId++ )
    {
    if ( ! (ptId % 10000) ) 
      {
      vtkDebugMacro(<<"Inserting vertex #" << ptId);
      this->UpdateProgress (0.25*ptId/npts);//25% spent inserting
      abortExecute = this->GetAbortExecute();
      }
    this->Insert(ptId);
    }
  this->UpdateProgress (0.25);//25% spent inserting
  
  CollapseTris = vtkIdList::New();
  CollapseTris->Allocate(100,100);
  
  // While the priority queue is not empty, retrieve the top vertex from the
  // queue and attempt to delete it by performing an edge collapse. This 
  // in turn will cause modification to the surrounding vertices. For each
  // surrounding vertex, evaluate the error and re-insert into the queue. 
  // (While this is happening we keep track of operations on the data - 
  // this forms the core of the progressive mesh representation.)
  for ( totalEliminated=0, reduction=0.0, numRecycles=0, numPops=0;
        reduction < this->TargetReduction && (ptId = this->Pop(error)) >= 0 && !abortExecute; 
        numPops++)
    {
    if ( numPops && !(numPops % 5000) )
      {
      vtkDebugMacro(<<"Deleting vertex #" << numPops);
      this->UpdateProgress (0.25 + 0.75*(reduction/this->TargetReduction));
      abortExecute = this->GetAbortExecute();
      }
    
    this->Mesh->GetPoint(ptId,this->X);
    this->Mesh->GetPointCells(ptId,ncells,cells);
    
    if ( ncells > 0 )
      {
      type = this->EvaluateVertex(ptId, ncells, cells, fedges);
      
      // FindSplit finds the edge to collapse - and if it fails, we
      // split the vertex.
      collapseId = this->FindSplit (type, fedges, pt1, pt2, CollapseTris);
      
      if ( collapseId >= 0 )
        {
        if ( this->AccumulateError )
          {
          this->DistributeError(error);
          }
        
        totalEliminated += this->CollapseEdge(type, ptId, collapseId, pt1, pt2,
                                              CollapseTris);
        
        reduction = static_cast<double>(totalEliminated) / numTris;
        this->NumberOfRemainingTris = numTris - totalEliminated;
        
        //see whether we've found inflection
        if ( numPops == 0 || (previousError == 0.0 && error != 0.0) ||
             (previousError != 0.0 && 
              fabs(error/previousError) > this->InflectionPointRatio) )
          {
          this->InflectionPoints->InsertNextValue(numPops);
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
  
  CollapseTris->Delete();
  
  totalPts = this->Mesh->GetNumberOfPoints();
  vtkDebugMacro(<<"\n\tReduction " << reduction << " (" << numTris << " to " 
                << numTris - totalEliminated << " triangles)"
                <<"\n\tPerformed " << numPops << " vertex pops"
                <<"\n\tFound " << this->GetNumberOfInflectionPoints() 
                <<" inflection points"
                <<"\n\tPerformed " << totalPts - numPts << " vertex splits"
                <<"\n\tPerformed " << this->NumCollapses << " edge collapses"
                <<"\n\tPerformed " << this->NumMerges << " vertex merges"
                <<"\n\tRecycled " << numRecycles << " points"
                <<"\n\tAdded " << totalPts - numPts << " points (" 
                << numPts << " to " << totalPts << " points)");

  //
  // Create output and release memory
  //
  vtkDebugMacro (<<"Creating output...");
  this->DeleteQueue();

  // Grab the points that are left; copy point data. Remember that splitting 
  // data may have added new points.
  map = new vtkIdType[totalPts];
  for (i=0; i < totalPts; i++)
    {
    map[i] = -1;
    }
  numNewPts = 0;
  for (ptId=0; ptId < totalPts; ptId++)
    {
    this->Mesh->GetPointCells(ptId,ncells,cells);
    if ( ncells > 0 )
      {
      map[ptId] = numNewPts++;
      }
    }
  
  outputPD->CopyAllocate(meshPD,numNewPts);

  // Copy points in place
  for (ptId=0; ptId < totalPts; ptId++)
    {
    if ( map[ptId] > -1 )
      {
      newPts->SetPoint(map[ptId],newPts->GetPoint(ptId));
      outputPD->CopyData(meshPD,ptId,map[ptId]);
      }
    }

  newPts->SetNumberOfPoints(numNewPts);
  newPts->Squeeze();

  // Now renumber connectivity
  newPolys = vtkCellArray::New();
  newPolys->Allocate(newPolys->EstimateSize(3,numTris-totalEliminated));

  for (cellId=0; cellId < numTris; cellId++)
    {
    if ( this->Mesh->GetCellType(cellId) == VTK_TRIANGLE ) // non-null element
      {
      this->Mesh->GetCellPoints(cellId, npts, pts);
      for (i=0; i < 3; i++)
        {
        newCellPts[i] = map[pts[i]];
        }
      newPolys->InsertNextCell(npts,newCellPts);
      }
    }

  delete [] map;
  output->SetPoints(newPts);
  output->SetPolys(newPolys);
  if (this->Mesh != NULL) {this->Mesh->Delete(); this->Mesh = NULL;}
  newPolys->Delete();

  return 1;
}

//
// Computes error to edge (distance squared)
//
static double ComputeEdgeError(double x[3], double x1[3], double x2[3])
{
  double projDist = vtkLine::DistanceToLine(x, x1, x2);
  double edgeLength = vtkMath::Distance2BetweenPoints(x1,x2);

  return (projDist < edgeLength ? projDist : edgeLength);
}

//
// Computes triangle area
//
static double ComputeSingleTriangleError(double x[3], double x1[3], double x2[3])
{
  return vtkTriangle::TriangleArea(x, x1, x2);
}

//
// Computes error to a cycle of triangles...the average plane (normal and
// point) have been already computed. (Returns distance squared.)
//
static double ComputeSimpleError(double x[3], double normal[3], double point[3])
{
  double dist = vtkPlane::DistanceToPlane(x, normal, point);
  return dist * dist;
}

// 
// Split the mesh along sharp edges - separates the mesh into pieces.
//
void vtkDecimatePro::SplitMesh()
{
  vtkIdType ptId, fedges[2];
  int type;
  vtkIdType *cells;
  unsigned short int ncells;

  this->CosAngle = cos( vtkMath::RadiansFromDegrees(  this->SplitAngle) );
  for ( ptId=0; ptId < this->Mesh->GetNumberOfPoints(); ptId++ )
    {
    this->Mesh->GetPoint(ptId,this->X);
    this->Mesh->GetPointCells(ptId,ncells,cells);

    if ( ncells > 0 && 
         ((type=this->EvaluateVertex(ptId,ncells,cells,fedges)) == VTK_CORNER_VERTEX ||
          type == VTK_INTERIOR_EDGE_VERTEX ||
          type == VTK_NON_MANIFOLD_VERTEX) )
      {
      this->SplitVertex(ptId, type, ncells, cells, 0);
      }
    }
}

#define VTK_FEATURE_ANGLE(tri1,tri2) \
                      vtkMath::Dot(this->T->Array[tri1].n, this->T->Array[tri2].n)
//
// Evalute the local topology/geometry of a vertex. This is a two-pass
// process: first topology is examined, and then the geometry.
//
int vtkDecimatePro::EvaluateVertex(vtkIdType ptId, unsigned short int numTris,
                                   vtkIdType *tris, vtkIdType fedges[2])
{
  vtkIdType numNei, numFEdges;
  vtkIdType numVerts;
  vtkDecimatePro::LocalTri t;
  vtkDecimatePro::LocalVertex sn;
  vtkIdType startVertex, nextVertex, numNormals;
  int i, j, vtype;
  vtkIdType *verts;
  double *x1, *x2, *normal;
  double v1[3], v2[3], center[3];
  //
  //  The first step is to evaluate topology.
  //

  // Check cases with high vertex degree
  //
  if ( numTris >= this->VertexDegree ) 
    {
    return VTK_HIGH_DEGREE_VERTEX;
    }

  //  From the adjacency structure we can find the triangles that use the
  //  vertex. Traverse this structure, gathering all the surrounding vertices
  //  into an ordered list.
  //
  this->V->Reset();
  this->T->Reset();

  sn.FAngle = 0.0;

  t.area = 0.0;
  t.n[0] = t.n[1] = t.n[2] = 0.0;
  t.verts[0] = -1; // Marks the fact that this poly hasn't been replaced 
  t.verts[1] = -1;
  t.verts[2] = -1;
  //
  //  Find the starting edge.  Do it very carefully do make sure
  //  ordering is consistent
  // (e.g., polygons ordering/normals remains consistent)
  //
  this->Mesh->GetCellPoints(*tris,numVerts,verts); // get starting point
  for (i=0; i<3; i++)
    {
    if (verts[i] == ptId)
      {
      break;
      }
    }
  sn.id = startVertex = verts[(i+1)%3];
  this->Mesh->GetPoint(sn.id, sn.x); //grab coordinates here to save GetPoint() calls

  this->V->InsertNextVertex(sn);

  nextVertex = -1; // initialize
  this->Neighbors->Reset();
  this->Neighbors->InsertId(0,*tris);
  numNei = 1;
  //
  //  Traverse the edge neighbors and see whether a cycle can be
  //  completed.  Also have to keep track of orientation of faces for
  //  computing normals.
  //
  while ( this->T->MaxId < numTris && numNei == 1 && nextVertex != startVertex) 
    {
    t.id = this->Neighbors->GetId(0);
    this->T->InsertNextTriangle(t);

    this->Mesh->GetCellPoints(t.id,numVerts,verts);
        
    for (j=0; j<3; j++) 
      {
      if (verts[j] != sn.id && verts[j] != ptId) 
        {
        nextVertex = verts[j];
        break;
        }
      }
    sn.id = nextVertex;
    this->Mesh->GetPoint(sn.id, sn.x);
    this->V->InsertNextVertex(sn);

    this->Mesh->GetCellEdgeNeighbors(t.id, ptId, nextVertex, this->Neighbors);
    numNei = this->Neighbors->GetNumberOfIds();
    } 
  //
  //  See whether we've run around the loop, hit a boundary, or hit a
  //  complex spot.
  //
  if ( nextVertex == startVertex && numNei == 1 ) 
    {
    if ( this->T->GetNumberOfTriangles() != numTris ) //touching non-manifold
      {
      vtype = VTK_NON_MANIFOLD_VERTEX;
      } 
    else  //remove last vertex addition
      {
      this->V->MaxId -= 1;
      vtype = VTK_SIMPLE_VERTEX;
      }
    }
  //
  //  Check for non-manifold cases
  //
  else if ( numNei > 1 || this->T->GetNumberOfTriangles() > numTris ) 
    {
    vtype = VTK_NON_MANIFOLD_VERTEX;
    }
  //
  //  Boundary loop - but (luckily) completed semi-cycle
  //
  else if ( numNei == 0 && this->T->GetNumberOfTriangles() == numTris ) 
    {
    this->V->Array[0].FAngle = -1.0; // using cosine of -180 degrees
    this->V->Array[this->V->MaxId].FAngle = -1.0;
    vtype = VTK_BOUNDARY_VERTEX;
    }
  //
  //  Hit a boundary but didn't complete semi-cycle.  Gotta go back
  //  around the other way.  Just reset the starting point and go 
  //  back the other way.
  //
  else 
    {
    t = this->T->GetTriangle(this->T->MaxId);

    this->V->Reset();
    this->T->Reset();

    startVertex = sn.id = nextVertex;
    this->Mesh->GetPoint(sn.id, sn.x);
    this->V->InsertNextVertex(sn);

    nextVertex = -1;
    this->Neighbors->Reset();
    this->Neighbors->InsertId(0,t.id);
    numNei = 1;
    //
    //  Now move from boundary edge around the other way.
    //
    while ( this->T->MaxId < numTris && numNei == 1 && nextVertex != startVertex) 
      {
      t.id = this->Neighbors->GetId(0);
      this->T->InsertNextTriangle(t);

      this->Mesh->GetCellPoints(t.id,numVerts,verts);
  
      for (j=0; j<3; j++) 
        {
        if (verts[j] != sn.id && verts[j] != ptId) 
          {
          nextVertex = verts[j];
          break;
          }
        }

      sn.id = nextVertex;
      this->Mesh->GetPoint(sn.id, sn.x);
      this->V->InsertNextVertex(sn);

      this->Mesh->GetCellEdgeNeighbors(t.id, ptId, nextVertex, this->Neighbors);
      numNei = this->Neighbors->GetNumberOfIds();
      }
    //
    //  Make sure that there are only two boundaries (i.e., not non-manifold)
    //
    if ( this->T->GetNumberOfTriangles() == numTris ) 
      {
      //
      //  Because we've reversed order of loop, need to rearrange the order
      //  of the vertices and polygons to preserve consistent polygons
      //  ordering / normal orientation.
      //
      numVerts = this->V->GetNumberOfVertices();
      for (i=0; i<(numVerts/2); i++) 
        {
        sn.id = this->V->Array[i].id;
        this->V->Array[i].id = this->V->Array[numVerts-i-1].id;
        this->V->Array[numVerts-i-1].id = sn.id;
        for (j=0; j<3; j++)
          {
          sn.x[j] = this->V->Array[i].x[j];
          this->V->Array[i].x[j] = this->V->Array[numVerts-i-1].x[j];
          this->V->Array[numVerts-i-1].x[j] = sn.x[j];
          }
        }

      numTris = this->T->GetNumberOfTriangles();
      for (i=0; i<(numTris/2); i++) 
        {
        t.id = this->T->Array[i].id;
        this->T->Array[i].id = this->T->Array[numTris-i-1].id;
        this->T->Array[numTris-i-1].id = t.id;
        }

      this->V->Array[0].FAngle = -1.0;
      this->V->Array[this->V->MaxId].FAngle = -1.0;
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
  x2 =  this->V->Array[0].x;
  for (i=0; i<3; i++)
    {
    v2[i] = x2[i] - this->X[i];
    }

  this->LoopArea=0.0;
  this->Normal[0] = this->Normal[1] = this->Normal[2] = 0.0;
  this->Pt[0] = this->Pt[1] = this->Pt[2] = 0.0;
  numNormals=0;

  for (i=0; i < this->T->GetNumberOfTriangles(); i++) 
    {
    normal = this->T->Array[i].n;
    x1 = x2;
    x2 = this->V->Array[i+1].x;

    for (j=0; j<3; j++) 
      {
      v1[j] = v2[j];
      v2[j] = x2[j] - this->X[j];
      }

    this->T->Array[i].area = vtkTriangle::TriangleArea (this->X, x1, x2);
    vtkTriangle::TriangleCenter (this->X, x1, x2, center);
    this->LoopArea += this->T->Array[i].area;

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
        this->Normal[j] += this->T->Array[i].area * normal[j];
        this->Pt[j] += this->T->Array[i].area * center[j];
        }
      }
    }
  //
  //  Compute "average" plane normal and plane center.  Use an area
  //  averaged normal calulation
  //
  if ( !numNormals || this->LoopArea == 0.0 ) 
    {
    return VTK_DEGENERATE_VERTEX;
    }

  for (j=0; j<3; j++) 
    {
    this->Normal[j] /= this->LoopArea;
    this->Pt[j] /= this->LoopArea;
    }
  if ( vtkMath::Normalize(this->Normal) == 0.0 )
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
    fedges[1] = this->V->MaxId;
    } 
  else
    {
    numFEdges = 0;
    }
  //
  //  Compare to cosine of feature angle to avoid cosine extraction
  //
  if ( vtype == VTK_SIMPLE_VERTEX ) // first edge 
    {
    if ( (this->V->Array[0].FAngle = VTK_FEATURE_ANGLE(0,this->T->MaxId)) <= this->CosAngle )
      {
      fedges[numFEdges++] = 0;
      }
    }

  for (i=0; i < this->T->MaxId; i++) 
    {
    if ( (this->V->Array[i+1].FAngle = VTK_FEATURE_ANGLE(i,i+1)) <= this->CosAngle ) 
      {
      if ( numFEdges >= 2 ) 
        {
        numFEdges++;
        }
      else 
        {
        fedges[numFEdges++] = i + 1;
        }
      }
    }
  //
  //  Final classification
  //
  if ( vtype == VTK_SIMPLE_VERTEX && numFEdges > 0 )
    {
    if ( numFEdges == 1 )
      {
      vtype = VTK_EDGE_END_VERTEX;
      }
    else if ( numFEdges == 2 )
      {
      vtype = VTK_INTERIOR_EDGE_VERTEX;
      }
    else
      {
      vtype = VTK_CORNER_VERTEX;
      }
    }
  else if ( vtype == VTK_BOUNDARY_VERTEX )
    {
    if ( numFEdges != 2 )
      {
      vtype = VTK_CORNER_VERTEX;
      }
    else
      {//see whether this is the tip of a crack
      if ( this->V->Array[fedges[0]].x[0] == this->V->Array[fedges[1]].x[0] &&
      this->V->Array[fedges[0]].x[1] == this->V->Array[fedges[1]].x[1] && 
      this->V->Array[fedges[0]].x[2] == this->V->Array[fedges[1]].x[2])
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
void vtkDecimatePro::SplitVertex(vtkIdType ptId, int type,
                                 unsigned short int numTris, vtkIdType *tris,
                                 int insert)
{
  vtkIdType id, fedge1, fedge2, i, j;
  vtkIdType tri, veryFirst;
  int numSplitTris;
  vtkIdType *verts, nverts;
  double error;
  vtkIdType startTri, p[2];
  int maxGroupSize;
  vtkPointData* meshPD = this->Mesh->GetPointData();

  //
  // On an interior edge split along the edge
  //
  if ( type == VTK_INTERIOR_EDGE_VERTEX ) //when edge splitting is on
    {
    // Half of loop is left connected to current vertex. Second half is
    // split away.
    for ( i=0; i < numTris; i++ ) // find first feature edge
      {
      if ( this->V->Array[i].FAngle <= this->CosAngle )
        {
        break;
        }
      }
    fedge1 = i;
    for ( i++, numSplitTris=1; this->V->Array[i].FAngle > this->CosAngle; i++ )
      {
      numSplitTris++;
      }

    fedge2 = i;

    // Now split region
    id = this->Mesh->InsertNextLinkedPoint(this->X,numSplitTris);
    meshPD->CopyData(meshPD, ptId, id);
    for ( i=fedge1; i < fedge2; i++ )
      { //disconnect from existing vertex
      tri = this->T->Array[i].id;
      this->Mesh->RemoveReferenceToCell(ptId, tri);
      this->Mesh->AddReferenceToCell(id, tri);
      this->Mesh->ReplaceCellPoint(tri, ptId, id);
      }

    // Compute error and insert the two vertices (old + split)
    error = ComputeEdgeError(this->X, this->V->Array[fedge1].x, this->V->Array[fedge2].x);
    if ( this->AccumulateError ) 
      {
      this->VertexError->InsertValue(id, this->VertexError->GetValue(ptId));
      }

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
    for ( i=0; i <= this->V->MaxId; i++ ) // find first feature edge
      {
      if ( this->V->Array[i].FAngle <= this->CosAngle && this->V->Array[i].FAngle != -1.0 ) 
        {
        break;
        }
      }
    for ( veryFirst = fedge1 = i; fedge1 < this->V->MaxId; i = fedge1 = fedge2 )
      {
      for (i++, numSplitTris=1; 
      i <= this->V->MaxId && this->V->Array[i].FAngle > this->CosAngle; i++)
        {
        numSplitTris++;
        }

      if ( (fedge2 = i) > this->V->MaxId )
        {
        continue; //must be part of first region
        }

      // Now split region
      id = this->Mesh->InsertNextLinkedPoint(this->X,numSplitTris);
      meshPD->CopyData(meshPD, ptId, id);

      for ( j=fedge1; j < fedge2; j++ )
        { //disconnect from existing vertex
        tri = this->T->Array[j].id;
        this->Mesh->RemoveReferenceToCell(ptId, tri);
        this->Mesh->AddReferenceToCell(id, tri);
        this->Mesh->ReplaceCellPoint(tri, ptId, id);
        }

      // Compute error for the vertex and insert
      error = ComputeEdgeError(this->X, this->V->Array[fedge1].x, this->V->Array[fedge2].x);
      if ( this->AccumulateError ) 
        {
        this->VertexError->InsertValue(id, this->VertexError->GetValue(ptId));
        }

      if ( insert )
        {
        this->Insert(id,error);    
        }
      }

    // don't forget to compute error for old vertex, and insert into queue
    if ( this->V->Array[0].FAngle == -1.0 )
      {
      error = ComputeEdgeError(this->X, this->V->Array[0].x, this->V->Array[veryFirst].x);
      }
    else
      {
      error = ComputeEdgeError(this->X, this->V->Array[veryFirst].x, this->V->Array[fedge1].x);
      }

    if ( insert )
      {
      this->Insert(ptId,error);    
      }
    }

  // Default case just splits off triangle(s) that form manifold groups. 
  // Note: this code also handles high-degree vertices.
  else
    {
    vtkIdList *triangles = vtkIdList::New();
    vtkIdList *cellIds = vtkIdList::New();
    vtkIdList *group = vtkIdList::New();

    triangles->Allocate(VTK_MAX_TRIS_PER_VERTEX);
    cellIds->Allocate(5,10);
    group->Allocate(VTK_MAX_TRIS_PER_VERTEX);

     //changes in group size control how to split loop
    if ( numTris <= 1 )
      {
      triangles->Delete();
      cellIds->Delete();
      group->Delete();
      return; //prevents infinite recursion
      }
    maxGroupSize = ( numTris < this->VertexDegree ? numTris : (this->VertexDegree - 1));

    if ( type == VTK_NON_MANIFOLD_VERTEX || type == VTK_HIGH_DEGREE_VERTEX )
      {
      ; //use maxGroupSize
      }
    else
      {
      maxGroupSize /= 2; //prevents infinite recursion
      }

    for ( i=0; i < numTris; i++ )
      {
      triangles->InsertId(i,tris[i]);
      }

    // now group into manifold pieces
    for ( i=0; triangles->GetNumberOfIds() > 0; i++ )
      {
      group->Reset();
      startTri = triangles->GetId(0);
      group->InsertId(0,startTri);
      triangles->DeleteId(startTri);
      this->Mesh->GetCellPoints(startTri,nverts,verts);
      p[0] = ( verts[0] != ptId ? verts[0] : verts[1] );
      p[1] = ( verts[1] != ptId && verts[1] != p[0] ? verts[1] : verts[2] );

      //grab manifold group - j index is the forward/backward direction around vertex
      for ( j=0; j < 2; j++ )
        {
        for ( tri=startTri; p[j] >= 0; )
          {
          this->Mesh->GetCellEdgeNeighbors(tri, ptId, p[j], cellIds);
          if ( cellIds->GetNumberOfIds() == 1 && 
               triangles->IsId((tri=cellIds->GetId(0))) > -1 && 
               group->GetNumberOfIds() < maxGroupSize )
            {
            group->InsertNextId(tri);
            triangles->DeleteId(tri);
            this->Mesh->GetCellPoints(tri,nverts,verts);
            if ( verts[0] != ptId && verts[0] != p[j] )
              {
              p[j] = verts[0];
              }
            else if ( verts[1] != ptId && verts[1] != p[j] )
              {
              p[j] = verts[1];
              }
            else
              {
              p[j] = verts[2];
              }
            }
          else
            {
            p[j] = -1;
            }
          }
        }//for both directions

      // reconnect group into manifold chunk (first group is left attached)
      if ( i != 0 ) 
        {
        id = this->Mesh->InsertNextLinkedPoint(this->X,group->GetNumberOfIds());
        meshPD->CopyData(meshPD, ptId, id);

        for ( j=0; j < group->GetNumberOfIds(); j++ )
          {
          tri = group->GetId(j);
          this->Mesh->RemoveReferenceToCell(ptId, tri);
          this->Mesh->AddReferenceToCell(id, tri);
          this->Mesh->ReplaceCellPoint(tri, ptId, id);
          }
        if ( this->AccumulateError ) 
          {
          this->VertexError->InsertValue(id, this->VertexError->GetValue(ptId));
          }
        if ( insert )
          {
          this->Insert(id);
          }
        }//if not first group
      }//for all groups
    //Don't forget to reinsert original vertex
    if ( insert )
      {
      this->Insert(ptId);
      }

    triangles->Delete();
    cellIds->Delete();
    group->Delete();
    }

  return;
}


//
// Find a way to split this loop. If -1 is returned, then we have a real
// bad situation and we'll split the vertex.
//
vtkIdType vtkDecimatePro::FindSplit (int type, vtkIdType fedges[2],
                                     vtkIdType& pt1, vtkIdType& pt2,
                                     vtkIdList *CollapseTris)
{
  vtkIdType i, maxI;
  double dist2, e2dist2;
  vtkIdType numVerts=this->V->MaxId+1;

  pt2 = -1;
  CollapseTris->SetNumberOfIds(2);
  this->EdgeLengths->Reset();

  switch (type)
    {
    case VTK_SIMPLE_VERTEX:
    case VTK_EDGE_END_VERTEX:
    case VTK_INTERIOR_EDGE_VERTEX:
      if ( type == VTK_INTERIOR_EDGE_VERTEX )
        {
        dist2 = vtkMath::Distance2BetweenPoints(this->X,
                                                this->V->Array[fedges[0]].x);
        this->EdgeLengths->Insert(dist2,fedges[0]);

        dist2 = vtkMath::Distance2BetweenPoints(this->X,
                                                this->V->Array[fedges[1]].x);
        this->EdgeLengths->Insert(dist2,fedges[1]);
        }
      else // Compute the edge lengths
        {
        for ( i=0; i < numVerts; i++ )
          {
          dist2 = vtkMath::Distance2BetweenPoints(this->X,
                                                  this->V->Array[i].x);
          this->EdgeLengths->Insert(dist2,i);
          }
        }

      // See whether the collapse is okay
      while ( (maxI = this->EdgeLengths->Pop(0, dist2)) >= 0 )
        {
        if ( this->IsValidSplit(maxI) )
          {
          break;
          }
        }

      if ( maxI >= 0 )
        {
        CollapseTris->SetId(0,this->T->Array[maxI].id);
        if ( maxI == 0 )
          {
          pt1 = this->V->Array[1].id;
          pt2 = this->V->Array[this->V->MaxId].id;
          CollapseTris->SetId(1,this->T->Array[this->T->MaxId].id);
          }
        else
          {
          pt1 = this->V->Array[(maxI+1)%numVerts].id;
          pt2 = this->V->Array[maxI-1].id;
          CollapseTris->SetId(1,this->T->Array[maxI-1].id);
          }

        return this->V->Array[maxI].id;
        }
      break;

    case VTK_BOUNDARY_VERTEX: //--------------------------------------------
      CollapseTris->SetNumberOfIds(1);
      // Compute the edge lengths
      dist2 = vtkMath::Distance2BetweenPoints(this->X, this->V->Array[0].x);
      e2dist2 = vtkMath::Distance2BetweenPoints(this->X,this->V->Array[this->V->MaxId].x);

      maxI = -1;
      if ( dist2 <= e2dist2 )
        {
        if ( this->IsValidSplit(0) )
          {
          maxI = 0;
          }
        else if ( this->IsValidSplit(this->V->MaxId) )
          {
          maxI = this->V->MaxId;
          }
        }
      else
        {
        if ( this->IsValidSplit(this->V->MaxId) )
          {
          maxI = this->V->MaxId;
          }
        else if ( this->IsValidSplit(0) )
          {
          maxI = 0;
          }
        }

      if ( maxI >= 0 )
        {
        if ( maxI == 0 )
          {
          CollapseTris->SetId(0,this->T->Array[0].id);
          pt1 = this->V->Array[1].id;
          return this->V->Array[0].id;
          }
        else
          {
          CollapseTris->SetId(0,this->T->Array[this->T->MaxId].id);
          pt1 = this->V->Array[this->V->MaxId-1].id;
          return this->V->Array[this->V->MaxId].id;
          }
        }
      break;

    case VTK_CRACK_TIP_VERTEX: //-------------------------------------------
      this->V->MaxId--;
      if ( this->IsValidSplit(0) )
        {
        CollapseTris->SetId(0,this->T->Array[0].id);
        pt1 = this->V->Array[1].id;
        pt2 = this->V->Array[this->V->MaxId].id;
        CollapseTris->SetId(1,this->T->Array[this->T->MaxId].id);
        return this->V->Array[0].id;
        }
      else
        {
        this->V->MaxId++;
        }
      break;

    case VTK_DEGENERATE_VERTEX: //-------------------------------------------
      // Collapse to the first edge
      CollapseTris->SetId(0,this->T->Array[0].id);
      pt1 = this->V->Array[1].id;
      if ( this->T->MaxId > 0 ) //more than one triangle
        {
        if ( this->T->MaxId == this->V->MaxId ) //a complete cycle
          {
          CollapseTris->SetId(1,this->T->Array[this->T->MaxId].id);
          pt2 = this->V->Array[this->V->MaxId].id;
          }
        else
          {
          CollapseTris->SetNumberOfIds(1);
          }
        }
      else
        {
        CollapseTris->SetNumberOfIds(1);
        }
      return this->V->Array[0].id;

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
  vtkIdType fedges[2];
  int i, sign;
  vtkIdType nverts=this->V->MaxId+1, j;
  double *x, val, sPt[3], v21[3], sN[3];
  vtkIdType l1[VTK_MAX_TRIS_PER_VERTEX], l2[VTK_MAX_TRIS_PER_VERTEX];
  vtkIdType n1, n2;

  // For a edge collapse to be valid, all edges to that vertex must
  // divide the loop cleanly.
  fedges[0] = index;
  for ( j=0; j < (nverts-3); j++ )
    {
    fedges[1] = (index + 2 + j) % nverts;
    this->SplitLoop (fedges, n1, l1, n2, l2);

    //  Create splitting plane.  Splitting plane is parallel to the loop
    //  plane normal and contains the splitting vertices fedges[0] and fedges[1].
    for (i=0; i<3; i++) 
      {
      sPt[i] = this->V->Array[fedges[0]].x[i];
      v21[i] = this->V->Array[fedges[1]].x[i] - sPt[i];
      }

    vtkMath::Cross (v21,this->Normal,sN);
    if ( vtkMath::Normalize(sN) == 0.0 )
      {
      return 0;
      }

    for (sign=0, i=0; i < n1; i++) // first loop 
      {
      if ( !(l1[i] == fedges[0] || l1[i] == fedges[1]) ) 
        {
        x = this->V->Array[l1[i]].x;
        val = vtkPlane::Evaluate(sN,sPt,x);
        if ( fabs(val) < this->Tolerance )
          {
          return 0;
          }
        
        if ( !sign )
          {
          sign = (val > this->Tolerance ? 1 : -1);
          }
        else if ( sign != (val > 0 ? 1 : -1) )
          {
          return 0;
          }
        }
      }

    sign *= -1;
    for (i=0; i < n2; i++) // second loop 
      {
      if ( !(l2[i] == fedges[0] || l2[i] == fedges[1]) ) 
        {
        x = this->V->Array[l2[i]].x;
        val = vtkPlane::Evaluate(sN,sPt,x);
        if ( fabs(val) < this->Tolerance )
          {
          return 0;
          }
        if ( !sign )
          {
          sign = (val > this->Tolerance ? 1 : -1);
          }
        else if ( sign != (val > 0 ? 1 : -1) )
          {
          return 0;
          }
        }
      }
    }// Check all splits
  return 1;
}

//
//  Creates two loops from splitting plane provided
//
void vtkDecimatePro::SplitLoop(vtkIdType fedges[2], vtkIdType& n1,
                               vtkIdType *l1, vtkIdType& n2, vtkIdType *l2)
{
  vtkIdType i;
  vtkIdType *loop;
  vtkIdType *count;

  n1 = n2 = 0;
  loop = l1;
  count = &n1;

  for (i=0; i <= this->V->MaxId; i++) 
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
int vtkDecimatePro::CollapseEdge(int type, vtkIdType ptId,
                                 vtkIdType collapseId, vtkIdType pt1, 
                                 vtkIdType pt2, vtkIdList *CollapseTris)
{
  vtkIdType i, numDeleted=CollapseTris->GetNumberOfIds();
  vtkIdType ntris=this->T->MaxId+1;
  vtkIdType nverts=this->V->MaxId+1;
  vtkIdType tri[2];
  vtkIdType verts[VTK_MAX_TRIS_PER_VERTEX+1];

  this->NumCollapses++;
  for ( i=0; i < numDeleted; i++ ) 
    {
    tri[i] = CollapseTris->GetId(i);
    }

  // type == VTK_CRACK_TIP_VERTEX || type == VTK_SIMPLE_VERTEX
  if ( numDeleted == 2 ) 
    {
    if ( type == VTK_CRACK_TIP_VERTEX ) //got to seal the crack first
      {
      this->NumMerges++;
      this->Mesh->RemoveReferenceToCell(this->V->Array[this->V->MaxId+1].id, tri[1]);
      this->Mesh->ReplaceCellPoint(tri[1],this->V->Array[this->V->MaxId+1].id, collapseId);
      }

    // delete two triangles
    this->Mesh->RemoveReferenceToCell(pt1, tri[0]);
    this->Mesh->RemoveReferenceToCell(pt2, tri[1]);
    this->Mesh->RemoveReferenceToCell(collapseId, tri[0]);
    this->Mesh->RemoveReferenceToCell(collapseId, tri[1]);
    this->Mesh->DeletePoint(ptId);
    this->Mesh->DeleteCell(tri[0]); this->Mesh->DeleteCell(tri[1]);

    // update topology to reflect new attachments
    this->Mesh->ResizeCellList(collapseId, ntris-2);
    for ( i=0; i < ntris; i++ )
      {
      if ( this->T->Array[i].id != tri[0] && this->T->Array[i].id != tri[1] )
        {
        this->Mesh->AddReferenceToCell(collapseId, this->T->Array[i].id);
        this->Mesh->ReplaceCellPoint(this->T->Array[i].id,ptId,collapseId);
        }
      }
    }//if interior vertex

  else // if ( numDeleted == 1 ) e.g., VTK_BOUNDARY_VERTEX
    {
    // delete one triangle
    this->Mesh->RemoveReferenceToCell(pt1, tri[0]);
    this->Mesh->RemoveReferenceToCell(collapseId, tri[0]);
    this->Mesh->DeletePoint(ptId);
    this->Mesh->DeleteCell(tri[0]);

    // update topology to reflect new attachments
    if ( ntris > 1 )
      {
      this->Mesh->ResizeCellList(collapseId, ntris-1);
      for ( i=0; i < ntris; i++ )
        {
        if ( this->T->Array[i].id != tri[0] )
          {
          this->Mesh->AddReferenceToCell(collapseId, this->T->Array[i].id);
          this->Mesh->ReplaceCellPoint(this->T->Array[i].id,ptId,collapseId);
          }
        }
      }
    } //else boundary vertex

  // Update surrounding vertices. Need to copy verts first because the V/T
  // arrays might change as points are being reinserted.
  //
  for ( i=0; i < nverts; i++ )
    {
    verts[i] = this->V->Array[i].id;
    }
  for ( i=0; i < nverts; i++ )
    {
    this->DeleteId(verts[i]);
    this->Insert(verts[i]);
    }

  return numDeleted;
}

// Get a list of inflection points. These are double values 0 < r <= 1.0 
// corresponding to reduction level, and there are a total of
// NumberOfInflectionPoints() values. You must provide an array (of
// the correct size) into which the inflection points are written.
void vtkDecimatePro::GetInflectionPoints(double *inflectionPoints)
{
  vtkIdType i;

  for (i=0; i < this->GetNumberOfInflectionPoints(); i++)
    {
    inflectionPoints[i] = this->InflectionPoints->GetValue(i);
    }
}

// Get a list of inflection points. These are double values 0 < r <= 1.0 
// corresponding to reduction level, and there are a total of
// NumberOfInflectionPoints() values. You must provide an array (of
// the correct size) into which the inflection points are written.
// This method returns a pointer to a list of inflection points.
double *vtkDecimatePro::GetInflectionPoints()
{
  return this->InflectionPoints->GetPointer(0);
}

// Get the number of inflection points. Only returns a valid value
// after the filter has executed.
vtkIdType vtkDecimatePro::GetNumberOfInflectionPoints()
{
  return this->InflectionPoints->GetMaxId()+1;
}

//
// The following are private functions used to manage the priority
// queue of vertices.
//

void vtkDecimatePro::InitializeQueue(vtkIdType numPts)
{
  if ( !this->PreserveTopology && this->Splitting ) 
    {
    numPts = static_cast<vtkIdType>(numPts*1.25);
    }

  this->Queue = vtkPriorityQueue::New();
  this->Queue->Allocate(numPts, static_cast<vtkIdType>(0.25*numPts));
}

int vtkDecimatePro::Pop(double &error)
{
  vtkIdType ptId;

  // Try returning what's in queue
  if ( (ptId = this->Queue->Pop(0, error)) >= 0 )
    {
    if ( error > this->Error )
      {
      this->Queue->Reset();
      }
    else
      {
      return ptId;
      }
    }

  // See whether anything's left and split/re-insert if allowed
  if ( this->NumberOfRemainingTris > 0 && this->Split &&
  this->SplitState == VTK_STATE_UNSPLIT )
    {
    vtkDebugMacro(<<"Splitting this->Mesh");

    this->SplitState = VTK_STATE_SPLIT;
    this->SplitMesh();
    this->CosAngle = cos( vtkMath::RadiansFromDegrees( this->SplitAngle ) );

    // Now that things are split, insert the vertices. (Have to do this
    // otherwise error calculation is incorrect.)
    for ( ptId=0; ptId < this->Mesh->GetNumberOfPoints(); ptId++ )
      {
      this->Insert(ptId);
      }

    if ( (ptId = this->Queue->Pop(0, error)) >= 0 )
      {
      if ( error > this->Error )
        {
        this->Queue->Reset();
        }
      else
        {
        return ptId;
        }
      }
    }

  // If here, then this->Mesh splitting hasn't helped or is exhausted. Run thru
  // vertices and split them as necessary no matter what.
  if ( this->NumberOfRemainingTris > 0 && this->Split && this->SplitState != VTK_STATE_SPLIT_ALL )
    {
    vtkDebugMacro(<<"Final splitting attempt");

    this->SplitState = VTK_STATE_SPLIT_ALL;
    for ( ptId=0; ptId < this->Mesh->GetNumberOfPoints(); ptId++ )
      {
      this->Insert(ptId);
      }

    if ( (ptId = this->Queue->Pop(0, error)) >= 0 )
      {
      if ( error > this->Error )
        {
        this->Queue->Reset();
        }
      else
        {
        return ptId;
        }
      }
    }

  //every possible point has been processed
  return -1; 
}

// Computes error and inserts point into priority queue.
void vtkDecimatePro::Insert(vtkIdType ptId, double error)
{
  int type, simpleType;
  vtkIdType *cells;
  vtkIdType fedges[2];
  unsigned short int ncells;

  // on value of error, we need to compute it or just insert the point
  if ( error < -this->Tolerance )
    {
    this->Mesh->GetPoint(ptId,this->X);
    this->Mesh->GetPointCells(ptId,ncells,cells);

    if ( ncells > 0 )
      {
      simpleType = 0;
      type = this->EvaluateVertex(ptId, ncells, cells, fedges);

      // Compute error for simple types - split vertex handles others
      if ( type == VTK_SIMPLE_VERTEX || type == VTK_EDGE_END_VERTEX ||
      type == VTK_CRACK_TIP_VERTEX )
        {
        simpleType = 1;
        error = ComputeSimpleError(this->X,this->Normal,this->Pt);
        }

      else if ( (type == VTK_INTERIOR_EDGE_VERTEX) ||
      (type == VTK_BOUNDARY_VERTEX && this->BoundaryVertexDeletion) )
        {
        simpleType = 1;
        if ( ncells == 1 ) //compute better error for single triangle 
          {
          error = ComputeSingleTriangleError(this->X,this->V->Array[0].x, 
                                             this->V->Array[1].x);
          }
        else
          {
          error = ComputeEdgeError(this->X, this->V->Array[fedges[0]].x, 
                                   this->V->Array[fedges[1]].x);
          }
        }

      if ( simpleType )
        {
        if ( this->AccumulateError )
          {
            error += this->VertexError->GetValue(ptId);
          }
        this->Queue->Insert(error,ptId);
        }

      // Type is complex so we break it up (if splitting allowed). A 
      // side-effect of splitting a vertex is that it inserts it and any 
      // new vertices into queue.
      else if ( this->SplitState == VTK_STATE_SPLIT && type != VTK_DEGENERATE_VERTEX )
        {
        this->SplitVertex(ptId, type, ncells, cells, 1);
        } //not a simple type

      } //if cells attached to vertex
    } //need to compute the error

  // If point is being recycled, see whether we want to split it
  else if ( error >= VTK_RECYCLE_VERTEX )
    {
    //see whether to split it, otherwise it isn't inserted yet
    if ( this->SplitState == VTK_STATE_SPLIT_ALL )
      {
      this->Mesh->GetPoint(ptId,this->X);
      this->Mesh->GetPointCells(ptId,ncells,cells);
      if ( ncells > 0 ) 
        {
        type = this->EvaluateVertex(ptId, ncells, cells, fedges);
        this->SplitVertex(ptId, type, ncells, cells, 1);
        }
      }
    }

  // Sometimes the error is computed for us so we insert it appropriately
  else 
    {
    if ( this->AccumulateError )
      {
        error += this->VertexError->GetValue(ptId);
      }
    this->Queue->Insert(error,ptId);
    }
}

// Compute the error of the point to the new triangulated surface
void vtkDecimatePro::DistributeError(double error)
{
  vtkIdType i;
  vtkIdType nverts=this->V->MaxId+1;
  double previousError;

  for (i=0; i < nverts; i++)
    {
    previousError = this->VertexError->GetValue(this->V->Array[i].id);
    this->VertexError->SetValue(this->V->Array[i].id, previousError+error);
    }
}
void vtkDecimatePro::DeleteQueue()
{
  if (this->Queue)
    {
    this->Queue->Delete();
    }
  this->Queue=NULL;
}

double vtkDecimatePro::DeleteId(vtkIdType id) 
{
  return this->Queue->DeleteId(id);
}

void vtkDecimatePro::Reset() 
{
  this->Queue->Reset();
}

void vtkDecimatePro::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Target Reduction: " << this->TargetReduction << "\n";
  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";

  os << indent << "Splitting: " << (this->Splitting ? "On\n" : "Off\n");
  os << indent << "Split Angle: " << this->SplitAngle << "\n";
  os << indent << "Pre-Split Mesh: "  
     << (this->PreSplitMesh ? "On\n" : "Off\n");

  os << indent << "Degree: " << this->Degree << "\n";

  os << indent << "Preserve Topology: " 
     << (this->PreserveTopology ? "On\n" : "Off\n");
  os << indent << "Maximum Error: "     << this->MaximumError << "\n";
  os << indent << "Accumulate Error: "  
     << (this->AccumulateError ? "On\n" : "Off\n");
  os << indent << "Error is Absolute: " 
     << (this->ErrorIsAbsolute ? "On\n" : "Off\n");
  os << indent << "Absolute Error: "    << this->AbsoluteError << "\n";

  os << indent << "Boundary Vertex Deletion: "  
     << (this->BoundaryVertexDeletion ? "On\n" : "Off\n");

  os << indent << "Inflection Point Ratio: " 
     << this->InflectionPointRatio << "\n";
  os << indent << "Number Of Inflection Points: "
     << this->GetNumberOfInflectionPoints() << "\n";
}
