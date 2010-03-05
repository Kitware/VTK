/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPolyhedron.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyhedron.h"

#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedTriangulator.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkQuad.h"
#include "vtkPolygon.h"
#include "vtkLine.h"
#include "vtkEdgeTable.h"
#include "vtkPolyData.h"
#include "vtkCellLocator.h"
#include "vtkGenericCell.h"
#include "vtkMeanValueCoordinatesInterpolator.h"

#include <vtkstd/map>

vtkCxxRevisionMacro(vtkPolyhedron, "$Revision: 1.7 $");
vtkStandardNewMacro(vtkPolyhedron);

// Special typedef for point id map
struct vtkPointIdMap : public vtkstd::map<vtkIdType,vtkIdType> {};
typedef vtkstd::map<vtkIdType,vtkIdType*>::iterator PointIdMapIterator;

//----------------------------------------------------------------------------
// Construct the hexahedron with eight points.
vtkPolyhedron::vtkPolyhedron()
{
  this->Line = vtkLine::New();
  this->Triangle = vtkTriangle::New();
  this->Quad = vtkQuad::New();
  this->Polygon = vtkPolygon::New();
  this->Tetra = vtkTetra::New();
  this->GlobalFaces = vtkIdTypeArray::New();
  this->FaceLocations = vtkIdTypeArray::New();
  this->PointIdMap = new vtkPointIdMap;
  
  this->EdgesGenerated = 0;
  this->EdgeTable = vtkEdgeTable::New();
  this->Edges = vtkIdTypeArray::New();
  this->Edges->SetNumberOfComponents(2);

  this->FacesGenerated = 0;
  this->Faces = vtkIdTypeArray::New();

  this->BoundsComputed = 0;
  
  this->PolyDataConstructed = 0;
  this->PolyData = vtkPolyData::New();
  this->Polys = vtkCellArray::New();
  this->PolyConnectivity = vtkIdTypeArray::New();
  this->LocatorConstructed = 0;
  this->CellLocator = vtkCellLocator::New();
  this->CellIds = vtkIdList::New();
  this->Cell = vtkGenericCell::New();
}

//----------------------------------------------------------------------------
vtkPolyhedron::~vtkPolyhedron()
{
  this->Line->Delete();
  this->Triangle->Delete();
  this->Quad->Delete();
  this->Polygon->Delete();
  this->Tetra->Delete();
  this->GlobalFaces->Delete();
  this->FaceLocations->Delete();
  delete this->PointIdMap;
  this->EdgeTable->Delete();
  this->Edges->Delete();
  this->Faces->Delete();
  this->PolyData->Delete();
  this->Polys->Delete();
  this->PolyConnectivity->Delete();
  this->CellLocator->Delete();
  this->CellIds->Delete();
  this->Cell->Delete();
}

//----------------------------------------------------------------------------
void vtkPolyhedron::ComputeBounds()
{
  if ( this->BoundsComputed )
    {
    return;
    }

  this->Superclass::GetBounds(); //stored in this->Bounds
  this->BoundsComputed = 1;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::ConstructPolyData()
{
  if (this->PolyDataConstructed)
    {
    return;
    }
  
  // Here's a trick, we're going to use the Faces array as the connectivity 
  // array. Note that the Faces have an added nfaces value at the beginning
  // of the array. Other than that,it's a vtkCellArray. So we play games 
  // with the pointers.
  this->GenerateFaces();
  
  this->PolyConnectivity->
    SetArray(this->Faces->GetPointer(1), this->Faces->GetSize()-1, 1);
  this->Polys->
    SetCells(*(this->Faces->GetPointer(0)), this->PolyConnectivity);

  // Standard setup
  this->PolyData->SetPoints(this->Points);
  this->PolyData->SetPolys(this->Polys);

  this->PolyDataConstructed = 1;
}


//----------------------------------------------------------------------------
void vtkPolyhedron::ConstructLocator()
{
  if (this->LocatorConstructed)
    {
    return;
    }
  
  this->ConstructPolyData();
  
  // With the polydata set up, we can assign it to the  locator
  this->CellLocator->SetDataSet(this->PolyData);
  this->CellLocator->BuildLocator();

  this->LocatorConstructed = 1;
}


//----------------------------------------------------------------------------
void vtkPolyhedron::ComputeParametricCoordinate(double x[3], double pc[3])
{
  this->ComputeBounds();
  double *bounds = this->Bounds;
  pc[0] = (x[0] - bounds[0]) / (bounds[1] - bounds[0]);
  pc[1] = (x[1] - bounds[2]) / (bounds[3] - bounds[2]);
  pc[2] = (x[2] - bounds[4]) / (bounds[5] - bounds[4]);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::
ComputePositionFromParametricCoordinate(double pc[3], double x[3])
{
  this->ComputeBounds();
  double *bounds = this->Bounds;
  x[0] = ( 1 - pc[0] )* bounds[0] + pc[0] * bounds[1];
  x[1] = ( 1 - pc[1] )* bounds[2] + pc[1] * bounds[3];
  x[2] = ( 1 - pc[2] )* bounds[4] + pc[2] * bounds[5];
}

//----------------------------------------------------------------------------
// Should be called by GetCell() prior to any other method invocation and after the
// points, point ids, and faces have been loaded. 
void vtkPolyhedron::Initialize()
{
  // Clear out any remaining memory.
  this->PointIdMap->clear();

  // We need to create a reverse map from the point ids to their canonical cell
  // ids. This is a fancy way of saying that we have to be able to rapidly go
  // from a PointId[i] to the location i in the cell.
  vtkIdType i, id, numPointIds = this->PointIds->GetNumberOfIds();
  for (i=0; i < numPointIds; ++i)
    {
    id = this->PointIds->GetId(i);
    (*this->PointIdMap)[id] = i;
    }
  
  // Edges have to be reset
  this->EdgesGenerated = 0;
  this->EdgeTable->Reset();
  this->Edges->Reset();
  this->Faces->Reset();
  
  // Faces may need renumbering later. This means converting the face ids from
  // global ids to local, canonical ids.
  this->FacesGenerated = 0;

  // No bounds have been computed as of yet.
  this->BoundsComputed = 0;
  
  // No supplemental geometric stuff created
  this->PolyDataConstructed = 0;
  this->LocatorConstructed = 0;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GetNumberOfEdges()
{
  // Make sure edges have been generated.
  if ( ! this->EdgesGenerated )
    {
    this->GenerateEdges();
    }
  
  return this->Edges->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
// This method requires that GenerateEdges() is invoked beforehand.
vtkCell *vtkPolyhedron::GetEdge(int edgeId)
{
  // Make sure edges have been generated.
  if ( ! this->EdgesGenerated )
    {
    this->GenerateEdges();
    }
  
  // Make sure requested edge is within range
  vtkIdType numEdges = this->Edges->GetNumberOfTuples();

  if ( edgeId < 0 || edgeId >= numEdges )
    {
    return NULL;
    }
  
  // Return the requested edge
  vtkIdType p, edge[2];
  this->Edges->GetTupleValue(edgeId,edge);

  // Recall that edge tuples are stored in canonical numbering
  for (int i=0; i<2; i++)
    {
    this->Line->PointIds->SetId(i,this->PointIds->GetId(edge[i]));
    this->Line->Points->SetPoint(i,this->Points->GetPoint(edge[i]));
    }

  return this->Line;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GenerateEdges()
{
  if ( this->EdgesGenerated )
    {
    return this->Edges->GetNumberOfTuples();
    }

  //check the number of faces and return if there aren't any
  if ( this->GlobalFaces->GetValue(0) <= 0 ) 
    {
    return 0;
    }
  
  // Loop over all faces, inserting edges into the table
  vtkIdType *faces = this->GlobalFaces->GetPointer(0);
  vtkIdType nfaces = faces[0];
  vtkIdType *face = faces + 1;
  vtkIdType fid, i, edge[2], npts;

  this->EdgeTable->InitEdgeInsertion(this->Points->GetNumberOfPoints());
  for (fid=0; fid < nfaces; ++fid)
    {
    npts = face[0];
    for (i=1; i <= npts; ++i)
      {
      edge[0] = (*this->PointIdMap)[face[i]];
      edge[1] = (*this->PointIdMap)[(i != npts ? face[i+1] : face[1])];
      if ( this->EdgeTable->IsEdge(edge[0],edge[1]) == (-1) )
        {
        this->EdgeTable->InsertEdge(edge[0],edge[1]);
        this->Edges->InsertNextTupleValue(edge);
        }
      }
    face += face[0] + 1;
    } //for all faces

  // Okay all done
  this->EdgesGenerated = 1;
  return this->Edges->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
int vtkPolyhedron::GetNumberOfFaces()
{
  return this->GlobalFaces->GetValue(0);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::GenerateFaces()
{
  if ( this->FacesGenerated )
    {
    return;
    }

  // Basically we just ron through the faces and change the global ids to the
  // canonical ids using the PointIdMap.
  this->Faces->SetNumberOfTuples(this->GlobalFaces->GetNumberOfTuples());
  vtkIdType *gFaces = this->GlobalFaces->GetPointer(0);
  vtkIdType *faces = this->Faces->GetPointer(0);
  vtkIdType nfaces = gFaces[0]; faces[0] = nfaces;
  vtkIdType *gFace = gFaces + 1;
  vtkIdType *face = faces + 1;
  vtkIdType fid, i, id, npts;

  for (fid=0; fid < nfaces; ++fid)
    {
    npts = gFace[0];
    face[0] = npts;
    for (i=1; i <= npts; ++i)
      {
      id = (*this->PointIdMap)[gFace[i]];
      face[i] = id;
      }
    gFace += gFace[0] + 1;
    face += face[0] + 1;
    } //for all faces


  // Okay we've done the deed
  this->FacesGenerated = 1;
}

//----------------------------------------------------------------------------
vtkCell *vtkPolyhedron::GetFace(int faceId)
{
  if ( faceId < 0 || faceId >= this->GlobalFaces->GetValue(0) )
    {
    return NULL;
    }
  
  this->GenerateFaces();

  // Okay load up the polygon
  vtkIdType i, p, loc = this->FaceLocations->GetValue(faceId);
  vtkIdType *face = this->GlobalFaces->GetPointer(loc);
  
  this->Polygon->PointIds->SetNumberOfIds(face[0]);
  this->Polygon->Points->SetNumberOfPoints(face[0]);
    
  // grab faces in global id space
  for (i=0; i < face[0]; ++i)
    {
    this->Polygon->PointIds->SetId(i,face[i+1]);
    p = (*this->PointIdMap)[face[i+1]];
    this->Polygon->Points->SetPoint(i,this->Points->GetPoint(p));
    }

  return this->Polygon;
}

//----------------------------------------------------------------------------
// Specify the faces for this cell.
void vtkPolyhedron::SetFaces(vtkIdType *faces)
{
  // Set up face structure
  this->GlobalFaces->Reset();
  this->FaceLocations->Reset();
  
  vtkIdType nfaces = faces[0];
  this->FaceLocations->SetNumberOfValues(nfaces);

  this->GlobalFaces->InsertNextValue(nfaces);
  vtkIdType *face = faces + 1;
  vtkIdType faceLoc = 1;
  vtkIdType i, fid, npts;

  for (fid=0; fid < nfaces; ++fid)
    {
    npts = face[0];
    this->GlobalFaces->InsertNextValue(npts);
    for (i=1; i<=npts; ++i)
      {
      this->GlobalFaces->InsertNextValue(face[i]);
      }
    this->FaceLocations->SetValue(fid,faceLoc);

    faceLoc += face[0] + 1;
    face = faces + faceLoc;
    } //for all faces
}

//----------------------------------------------------------------------------
// Return the list of faces for this cell.
vtkIdType *vtkPolyhedron::GetFaces()
{
  return this->GlobalFaces->GetPointer(0);
}

//----------------------------------------------------------------------------
int vtkPolyhedron::IntersectWithLine(double p1[3], double p2[3], double tol, 
                                     double& tMin, double xMin[3], 
                                     double pc[3], int& subId)
{
  // It's easiest if this is done in canonical space
  this->GenerateFaces();

  // Loop over all the faces, intersecting them in turn.
  vtkIdType *face = this->Faces->GetPointer(0);
  vtkIdType nfaces = *face++;
  vtkIdType npts, i, fid, hit, numHits=0;
  double t=VTK_LARGE_FLOAT;
  double x[3];

  for (fid=0; fid < nfaces; ++fid)
    {
    npts = face[0];
    hit = 0;
    switch (npts)
      {
      case 3: //triangle
        for (i=0; i<3; i++)
          {
          this->Triangle->Points->SetPoint(i,this->Points->GetPoint(face[i+1]));
          this->Triangle->PointIds->SetId(i,face[i+1]);
          }
        hit = this->Triangle->IntersectWithLine(p1,p2,tol,t,x,pc,subId);
        break;
      case 4: //quad
        for (i=0; i<4; i++)
          {
          this->Quad->Points->SetPoint(i,this->Points->GetPoint(face[i+1]));
          this->Quad->PointIds->SetId(i,face[i+1]);
          }
        hit = this->Quad->IntersectWithLine(p1,p2,tol,t,x,pc,subId);
        break;
      default: //general polygon
        for (i=0; i<npts; i++)
          {
          this->Polygon->Points->SetPoint(i,this->Points->GetPoint(face[i+1]));
          this->Polygon->PointIds->SetId(i,face[i+1]);
          }
        hit = this->Polygon->IntersectWithLine(p1,p2,tol,t,x,pc,subId);
        break;
      }

    // Update minimum hit
    if ( hit )
      {
      numHits++;
      if ( t < tMin )
        {
        tMin = t;
        xMin[0] = x[0]; xMin[1] = x[1]; xMin[2] = x[2];
        }
      }

    face += face[0] + 1;
    }//for all faces

  // Compute parametric coordinates
  this->ComputeParametricCoordinate(xMin,pc);

  return numHits;
}

#define VTK_CERTAIN 1
#define VTK_UNCERTAIN 0
#define VTK_MAX_ITER 10    //Maximum iterations for ray-firing
#define VTK_VOTE_THRESHOLD 3

//----------------------------------------------------------------------------
// Shoot random rays and count the number of intersections
int vtkPolyhedron::IsInside(double x[3], double tolerance)
{
  // do a quick bounds check
  this->ComputeBounds();
  double *bounds = this->Bounds;
  if ( x[0] < bounds[0] || x[0] > bounds[1] ||
       x[1] < bounds[2] || x[1] > bounds[3] ||
       x[2] < bounds[4] || x[2] > bounds[5])
    {
    return 0;
    }
  
  // It's easiest if these computations are done in canonical space
  this->GenerateFaces();

  // This algorithm is adaptive; if there are enough faces in this
  // polyhedron, a cell locator is built to accelerate intersections.
  // Otherwise brute force looping over cells is used.
  vtkIdType *faceArray = this->Faces->GetPointer(0);
  vtkIdType nfaces = *faceArray++;
  if ( nfaces > 25 )
    {
    this->ConstructLocator();
    }

  // We need a length to normalize the computations
  double length = sqrt( this->Superclass::GetLength2() );

  //  Perform in/out by shooting random rays. Multiple rays are fired
  //  to improve accuracy of the result.
  //
  //  The variable iterNumber counts the number of rays fired and is
  //  limited by the defined variable VTK_MAX_ITER.
  //
  //  The variable deltaVotes keeps track of the number of votes for
  //  "in" versus "out" of the surface.  When deltaVotes > 0, more votes
  //  have counted for "in" than "out".  When deltaVotes < 0, more votes
  //  have counted for "out" than "in".  When the delta_vote exceeds or
  //  equals the defined variable VTK_VOTE_THRESHOLD, then the
  //  appropriate "in" or "out" status is returned.
  //
  double rayMag, ray[3], xray[3], t, pcoords[3], xint[3];
  int i, numInts, iterNumber, deltaVotes, subId;
  vtkIdType idx, numCells;
  double tol = tolerance * length;

  for (deltaVotes = 0, iterNumber = 1;
       (iterNumber < VTK_MAX_ITER) && (abs(deltaVotes) < VTK_VOTE_THRESHOLD);
       iterNumber++) 
    {
    //  Define a random ray to fire.
    rayMag = 0.0;
    while (rayMag == 0.0 )
      {
      for (i=0; i<3; i++)
        {
        ray[i] = vtkMath::Random(-1.0,1.0);
        }
      rayMag = vtkMath::Norm(ray);
      }

    // The ray must be appropriately sized wrt the bounding box. (It has to go
    // all the way through the bounding box.)
    for (i=0; i<3; i++)
      {
      xray[i] = x[i] + (length/rayMag)*ray[i];
      }

    // Intersect the line with each of the candidate cells
    numInts = 0;

    if ( this->LocatorConstructed )
      {
      // Retrieve the candidate cells from the locator
      this->CellLocator->FindCellsAlongLine(x,xray,tol,this->CellIds);
      numCells = this->CellIds->GetNumberOfIds();

      for ( idx=0; idx < numCells; idx++ )
        {
        this->PolyData->GetCell(this->CellIds->GetId(idx), this->Cell);
        if ( this->Cell->IntersectWithLine(x, xray, tol, t, xint, pcoords, subId) )
          {
          numInts++;
          }
        } //for all candidate cells
      }
    else
      {
      numCells = nfaces;
      this->ConstructPolyData();

      for ( idx=0; idx < numCells; idx++ )
        {
        this->PolyData->GetCell(idx, this->Cell);
        if ( this->Cell->IntersectWithLine(x, xray, tol, t, xint, pcoords, subId) )
          {
          numInts++;
          }
        } //for all candidate cells
      }
    
    // Count the result
    if ( (numInts % 2) == 0)
      {
      --deltaVotes;
      }
    else
      {
      ++deltaVotes;
      }
    } //try another ray

  //   If the number of votes is positive, the point is inside
  //
  return ( deltaVotes < 0 ? 0 : 1 );
}

#undef VTK_CERTAIN
#undef VTK_UNCERTAIN
#undef VTK_MAX_ITER
#undef VTK_VOTE_THRESHOLD


//----------------------------------------------------------------------------
int vtkPolyhedron::CellBoundary(int subId, double pcoords[3],
                                    vtkIdList *pts)
{ 
  return 0;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::EvaluatePosition( double x[3], double * closestPoint,
                                     int & vtkNotUsed(subId), double pcoords[3],
                                     double & minDist2, double * weights )
{
  // compute parametric coordinates 
  this->ComputeParametricCoordinate(x, pcoords);
  
  // construct polydata, the result is stored in this->PolyData,
  // the cell array is stored in this->Polys
  this->ConstructPolyData();
  
  // Construct cell locator
  this->ConstructLocator();
  
  // find closest point and store the squared distance
  vtkGenericCell * genCell = vtkGenericCell::New( );

  vtkIdType cellId;
  int id;
  this->CellLocator->FindClosestPoint(
    x, closestPoint, genCell, cellId, id, minDist2 );

  genCell->Delete();

  // set distance to be zero, if point is inside
  if (this->IsInside(x, VTK_DOUBLE_MIN))
    {
    minDist2 = 0.0;
    }
  
  // get the MVC weights
  if (this->PolyData->GetPoints())
    {
    return 0;
    }
  vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(
    x, this->PolyData->GetPoints(), this->Polys, weights);

  return 0;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::EvaluateLocation( int &  subId, double   pcoords[3], 
                                      double x[3],  double * weights )
{
  this->ComputePositionFromParametricCoordinate(pcoords, x);
  
  this->InterpolateFunctions(x, weights);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::Derivatives(int vtkNotUsed(subId), double pcoords[3],
                                double *values, int dim, double *derivs)
{
  int i, j, k, idx;
  for ( j = 0; j < dim; j++ )
    {
    for ( i = 0; i < 3; i++ )
      {
      derivs[j*dim + i] = 0.0;
      }
    }

  static const double Sample_Offset_In_Parameter_Space = 0.01;
  
  double x[4][3];
  double coord[3];
  
  //compute positions of poand three offset sample points
  coord[0] = pcoords[0];
  coord[1] = pcoords[1];
  coord[2] = pcoords[2];
  this->ComputePositionFromParametricCoordinate(coord, x[0]);
  
  coord[0] += Sample_Offset_In_Parameter_Space;
  this->ComputePositionFromParametricCoordinate(coord, x[1]);
  coord[0] = pcoords[0];  

  coord[1] += Sample_Offset_In_Parameter_Space;
  this->ComputePositionFromParametricCoordinate(coord, x[2]);
  coord[1] = pcoords[1];  

  coord[2] += Sample_Offset_In_Parameter_Space;
  this->ComputePositionFromParametricCoordinate(coord, x[3]);
  coord[2] = pcoords[2];  

  this->ConstructPolyData();
  int numVerts = this->PolyData->GetNumberOfPoints();

  double *weights = new double[numVerts];
  double *sample = new double[dim*4];
  //for each sample point, sample data values
  for ( idx = 0, k = 0; k < 4; k++ ) //loop over three sample points
    {
    this->InterpolateFunctions(x[k],weights);
    for ( j = 0; j < dim; j++, idx++) //over number of derivates requested
      {
      sample[idx] = 0.0;
      for ( i = 0; i < numVerts; i++ )
        {
        sample[idx] += weights[i] * values[j + i*dim];
        }
      }
    }

  double v1[3], v2[3], v3[3];
  double l1, l2, l3;
  //compute differences along the two axes
  for ( i = 0; i < 3; i++ )
    {
    v1[i] = x[1][i] - x[0][i];
    v2[i] = x[2][i] - x[0][i];
    v3[i] = x[3][i] - x[0][i];
    }
  l1 = vtkMath::Normalize(v1);
  l2 = vtkMath::Normalize(v2);
  l3 = vtkMath::Normalize(v3);
  

  //compute derivatives along x-y-z axes
  double ddx, ddy, ddz;
  for ( j = 0; j < dim; j++ )
    {
    ddx = (sample[  dim+j] - sample[j]) / l1;
    ddy = (sample[2*dim+j] - sample[j]) / l2;
    ddz = (sample[3*dim+j] - sample[j]) / l3;

    //project onto global x-y-z axes
    derivs[3*j]     = ddx*v1[0] + ddy*v2[0] + ddz*v3[0];
    derivs[3*j + 1] = ddx*v1[1] + ddy*v2[1] + ddz*v3[1];
    derivs[3*j + 2] = ddx*v1[2] + ddy*v2[2] + ddz*v3[2];
    }

  delete [] weights;
  delete [] sample;
}

//----------------------------------------------------------------------------
double *vtkPolyhedron::GetParametricCoords()
{
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::InterpolateFunctions(double x[3], double *sf)
{
  // construct polydata, the result is stored in this->PolyData,
  // the cell array is stored in this->Polys
  this->ConstructPolyData();

  // compute the weights
  if (!this->PolyData->GetPoints())
    {
    return;
    }
  vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(
    x, this->PolyData->GetPoints(), this->Polys, sf);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::InterpolateDerivs(double x[3], double *derivs)
{
}

//----------------------------------------------------------------------------
int vtkPolyhedron::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                               vtkPoints *pts)
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::Contour(double value,
                            vtkDataArray *cellScalars,
                            vtkIncrementalPointLocator *locator,
                            vtkCellArray *verts, vtkCellArray *lines,
                            vtkCellArray *polys,
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId,
                            vtkCellData *outCd)
{
}


//----------------------------------------------------------------------------
void vtkPolyhedron::Clip(double value,
                         vtkDataArray *cellScalars,
                         vtkIncrementalPointLocator *locator, vtkCellArray *tets,
                         vtkPointData *inPD, vtkPointData *outPD,
                         vtkCellData *inCD, vtkIdType cellId,
                         vtkCellData *outCD, int insideOut)
{
}


//----------------------------------------------------------------------------
void vtkPolyhedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Triangle:\n";
  this->Triangle->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Polygon:\n";
  this->Polygon->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Tetra:\n";
  this->Tetra->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Faces:\n";
  this->GlobalFaces->PrintSelf(os,indent.GetNextIndent());

}
