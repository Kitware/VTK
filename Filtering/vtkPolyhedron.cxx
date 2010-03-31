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
#include "vtkIncrementalPointLocator.h"
#include "vtkMeanValueCoordinatesInterpolator.h"
#include "vtkSmartPointer.h"
#include "vtkMergePoints.h"

#include <vtkstd/map>
#include <vtkstd/set>

vtkCxxRevisionMacro(vtkPolyhedron, "$Revision: 1.7 $");
vtkStandardNewMacro(vtkPolyhedron);

// Special typedef for point id map
struct vtkPointIdMap : public vtkstd::map<vtkIdType,vtkIdType> {};
typedef vtkstd::map<vtkIdType,vtkIdType*>::iterator PointIdMapIterator;


// Special class for iterating through polyhedron faces
class vtkPolyhedronFaceIterator
{
public:
  vtkIdType CurrentPolygonSize;
  vtkIdType *Polygon;
  vtkIdType *Current;
  vtkIdType NumberOfPolygons;
  vtkIdType Id;

  vtkPolyhedronFaceIterator(vtkIdType numFaces, vtkIdType *t)
    {
      this->CurrentPolygonSize = t[0];
      this->Polygon = t;
      this->Current = t+1;
      this->NumberOfPolygons = numFaces;
      this->Id = 0;
    }
  vtkIdType* operator++()
    {
      this->Current += this->CurrentPolygonSize + 1;
      this->Polygon = this->Current - 1;
      this->CurrentPolygonSize = *(Polygon);
      this->Id++;
      return this->Current;
    }
};


// Special class for iterating through vertices on a polygon face
class vtkPolygonVertexIterator
{
public:
  vtkIdType *Current;
  vtkIdType NumberOfVertices;
  vtkIdType Id;
  // 1 or 0 for iterating along its original direction or reverse
  vtkIdType IterDirection; 

  vtkPolygonVertexIterator(vtkIdType numVertices, vtkIdType startVertex, 
                           vtkIdType *startVertexPointer, vtkIdType nextVertex)
    {
    this->Current = startVertexPointer;
    this->NumberOfVertices = numVertices;
    this->Id = startVertex;
    this->IterDirection = 1;
    vtkIdType nextId = this->Id + 1;
    vtkIdType *next = this->Current + 1;
    if (nextId == this->NumberOfVertices)
      {
      next -= this->NumberOfVertices;
      }
    if (*next != nextVertex)
      {
      this->IterDirection = 0;
      }
    }

  vtkIdType* operator++()
    {
    if (this->IterDirection)
      {
      this->Id++;
      this->Current++;
      if (this->Id == this->NumberOfVertices)
        {
        this->Id = 0;
        this->Current -= this->NumberOfVertices;
        }
      }
    else
      {
      this->Id--;
      this->Current--;
      if (this->Id == -1)
        {
        this->Id = this->NumberOfVertices - 1;
        this->Current += this->NumberOfVertices;
        }
      }
    return this->Current;
    }
};

namespace
{
// insert new id element in between two existing adjacent id elements.
// this is a convenient function. no check whether the input elements 
// exist in the array. no check for element adjacency.
int InsertNewIdToIdArray(vtkstd::vector<vtkIdType> & idArray, 
                         vtkIdType id, vtkIdType id0, vtkIdType id1)
{
  if (idArray.size() < 2)
    {
    return 0;
    }
  
  size_t num = idArray.size();
  if ((idArray[0] == id0 && idArray[num-1] == id1)
    ||(idArray[0] == id1 && idArray[num-1] == id0))
    {
    idArray.push_back(id);
    return 1;
    }
  
  vtkstd::vector<vtkIdType>::iterator iter = idArray.begin();
  for (; iter != idArray.end(); ++iter)
    {
    if (*iter == id0 || *iter == id1)
      {
      ++iter;
      idArray.insert(iter, id);
      return 1;
      }
    }
  
  return 0;
}
} //end namespace


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
  
  this->TriangulationPerformed = 0;
  this->Tets = vtkIdList::New();
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
  this->Tets->Delete();
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
  vtkIdType edge[2];
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
  vtkSmartPointer<vtkGenericCell> genCell = 
    vtkSmartPointer<vtkGenericCell>::New( );

  vtkIdType cellId;
  int id;
  this->CellLocator->FindClosestPoint(
    x, closestPoint, genCell, cellId, id, minDist2 );

  // set distance to be zero, if point is inside
  if (this->IsInside(x, VTK_DOUBLE_MIN))
    {
    minDist2 = 0.0;
    }
  
  // get the MVC weights
  this->InterpolateFunctions(x, weights);

  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::EvaluateLocation( int & vtkNotUsed(subId), double pcoords[3], 
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
  
  //compute positions of point and three offset sample points
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
  (void)x;
  (void)derivs;
}

//----------------------------------------------------------------------------
int vtkPolyhedron::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                               vtkPoints *pts)
{
  ptIds->Reset();
  pts->Reset();
  
  if (this->TriangulationPerformed)
    {
    pts->DeepCopy(this->GetPoints());
    ptIds->DeepCopy(this->Tets);
    return 1;
    }
  
  if (!this->GetPoints() || !this->GetNumberOfPoints())
    {
    return 0;
    }
  
  this->ComputeBounds();
  
  // use ordered triangulator to triangulate the polyhedron.
  vtkSmartPointer<vtkOrderedTriangulator> triangulator = 
    vtkSmartPointer<vtkOrderedTriangulator>::New();
  
  triangulator->InitTriangulation(this->Bounds, this->GetNumberOfPoints());
  triangulator->PreSortedOff();
  
  double point[3], pcoord[3]; 
  for (int i = 0; i < this->GetNumberOfPoints(); i++)
    {
    this->GetPoints()->GetPoint(i, point);
    this->ComputeParametricCoordinate(point, pcoord);
    triangulator->InsertPoint(i, point, pcoord, 0);
    }
  triangulator->Triangulate();
  
  triangulator->AddTetras(0, ptIds, pts);
  
  this->Tets->DeepCopy(ptIds);  
  this->TriangulationPerformed = 1;

  return 1;
}

//----------------------------------------------------------------------------
void vtkPolyhedron::Contour(double value,
                            vtkDataArray *cellScalars,
                            vtkIncrementalPointLocator *locator,
                            vtkCellArray *vtkNotUsed(verts), 
                            vtkCellArray *vtkNotUsed(lines),
                            vtkCellArray *polys,
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId,
                            vtkCellData *outCd)
{
  // return if there is no edge
  if (!this->GenerateEdges())
    {
    return;
    }
  
  double x0[3], x1[3], x[3];
  double v0, v1, t;

  vtkIdType p0, p1, flag, pId, fId;
  void * ptr = NULL;

  // initialize point locator
  this->ConstructPolyData();
  this->ComputeBounds();
  vtkSmartPointer<vtkMergePoints> merge = 
    vtkSmartPointer<vtkMergePoints>::New();
  merge->InitPointInsertion(this->Points, this->Bounds);
  for (int i = 0; i < this->Points->GetNumberOfPoints(); i++)
    {
    merge->InsertUniquePoint(this->Points->GetPoint(i), pId);
    }
  
  outPd->DeepCopy(inPd);
  
  vtkIdType offset = polys->GetNumberOfCells();
  
  typedef vtkstd::map<vtkIdType, vtkstd::vector<vtkIdType> > 
                                                           IdToIdArrayMapType;
  typedef vtkstd::pair<vtkIdType, vtkstd::vector<vtkIdType> > 
                                                           IdToIdArrayPairType;
  typedef IdToIdArrayMapType::iterator              IdToIdArrayMapIteratorType;

  // map from a vertex to a vector of its adjacent faces
  IdToIdArrayMapType vfMap;
  IdToIdArrayMapIteratorType vfMapIt, vfMapIt0, vfMapIt1;

  // map from a face to a vector of vertices on it
  IdToIdArrayMapType fvMap;
  IdToIdArrayMapIteratorType fvMapIt;

  // loop through all faces
  vtkPolyhedronFaceIterator 
    faceIter(this->GetNumberOfFaces(), this->Faces->GetPointer(1));
  while ( faceIter.Id < faceIter.NumberOfPolygons)
    {
    // the rest code of this function assumes that a face contains at least 
    // three vertices. return if find a single-vertex or double-vertex face.
    if (faceIter.CurrentPolygonSize < 3)
      {
      vtkErrorMacro("Find a face with " << faceIter.CurrentPolygonSize <<
        " vertices. Contouring aborted due to this degenrate case.");
      return;      
      }

    fId = faceIter.Id;
    vtkstd::vector<vtkIdType> vArray;
    for (vtkIdType i = 0; i < faceIter.CurrentPolygonSize; i++)
      {
      pId = faceIter.Current[i];
      vfMapIt = vfMap.find(pId);
      if (vfMapIt != vfMap.end())
        {
        vfMapIt->second.push_back(fId);
        }
      else
        {
        vtkstd::vector<vtkIdType> fArray;
        fArray.push_back(fId);
        vfMap.insert(IdToIdArrayPairType(pId, fArray));
        }
      vArray.push_back(pId);
      }
    
    fvMap.insert(IdToIdArrayPairType(fId, vArray));
    ++faceIter;
    }

  const double eps = 0.00000001;

  // loop through all edges to find contour points and store them in the point 
  // locator. if the contour points are new (not overlap with any of original 
  // vertex), update vfMap and fvMap.
  vtkstd::set<vtkIdType> cpSet; // contour point set
  this->EdgeTable->InitTraversal();
  while (this->EdgeTable->GetNextEdge(p0, p1, ptr))
    {
    v0 = cellScalars->GetComponent(p0,0);
    v1 = cellScalars->GetComponent(p1,0);

    if (v1-value > eps && v0-value > eps || v1-value < -eps && v0-value < -eps)
      {
      continue;
      }
    
    flag = 0;
    if (v0 - value < eps && v0 - value > -eps)
      {
      cpSet.insert(p0);
      flag = 1;
      }
    if (v1 - value < eps && v1 - value > -eps)
      {
      cpSet.insert(p1);
      flag = 1;
      }
    
    if (flag)
      {
      continue;
      }
    
    this->Points->GetPoint(p0, x0);
    this->Points->GetPoint(p1, x1);

    t = (value - v0)/(v1 - v0);
    x[0] = (1 - t) * x0[0] + t * x1[0];
    x[1] = (1 - t) * x0[1] + t * x1[1];
    x[2] = (1 - t) * x0[2] + t * x1[2];

    if (merge->InsertUniquePoint(x, pId))
      {
      // find adjacent faces for the new point
      vfMapIt0 = vfMap.find(p0);
      vfMapIt1 = vfMap.find(p1);
      if (vfMapIt0 == vfMap.end() || vfMapIt1 == vfMap.end())
        {
        vtkErrorMacro("Cannot locate adjacent faces of a vertex. We should never "
          "get here. Contouring continues but may generate wrong result.");
        continue;
        }
      
      vtkstd::vector<vtkIdType> fArray0 = vfMapIt0->second;
      vtkstd::vector<vtkIdType> fArray1 = vfMapIt1->second;
      vtkstd::set<vtkIdType> fSet;
      for (size_t i = 0; i < fArray0.size(); i++)
        {
        for (size_t j = 0; j < fArray1.size(); j++)
          {
          if (fArray0[i] == fArray1[j])
            {
            fSet.insert(fArray0[i]);
            }
          }
        }
      if (fSet.empty())
        {
        vtkErrorMacro("Cannot locate adjacent faces of a contour point. We should "
          "never get here. Contouring continues but may generate wrong result.");
        continue;
        }

      // update vfMap: add adjacent faces to the new point
      vtkstd::vector<vtkIdType> fArray;
      vtkstd::set<vtkIdType>::iterator fSetIt;
      for (fSetIt = fSet.begin(); fSetIt != fSet.end(); ++fSetIt)
        {
        fArray.push_back(*fSetIt);
        }
      vfMap.insert(IdToIdArrayPairType(pId, fArray));
      
      // update fvMap: insert the new point to the adjacent faces
      for (fSetIt = fSet.begin(); fSetIt != fSet.end(); ++fSetIt)
        {
        fvMapIt = fvMap.find(*fSetIt);
        InsertNewIdToIdArray(fvMapIt->second, pId, p0, p1);
        }
      
      // maintain point data
      if (outPd)
        {
        outPd->GetScalars()->InsertTuple1(pId, value);
        }
      
      }
      
    cpSet.insert(pId);
    }
  
  int numberOfAllPoints = pId + 1;
  int numberOfContourPoints = cpSet.size();
  
  //
  // Construct result contour by connecting any two contour points on the 
  // same plane. If there are more than 2 contour points on one plane, connect
  // them in a order to avoid self-intersecting.
  
  vtkSmartPointer<vtkEdgeTable> polyEdgeTable = 
    vtkSmartPointer<vtkEdgeTable>::New();
  polyEdgeTable->InitEdgeInsertion(numberOfAllPoints);
  
  // array indicating faces with contour points
  bool *validFaces = new bool [this->GetNumberOfFaces()];
  for (int i = 0; i < this->GetNumberOfFaces(); i++)
    {
    validFaces[i] = false;
    }

  // array indicating contour points
  bool *validPoints = new bool [numberOfAllPoints];
  for (int i = 0; i < numberOfAllPoints; i++)
    {
    validPoints[i] = false;
    }
  
  // find valid contour points and their faces
  vtkstd::set<vtkIdType>::iterator cpSetIt;
  vtkstd::vector<vtkIdType> fArray;
  for (cpSetIt = cpSet.begin(); cpSetIt != cpSet.end(); ++cpSetIt)
    {
    validPoints[*cpSetIt] = true;
    
    fArray = vfMap.find(*cpSetIt)->second;
    for (size_t i = 0; i < fArray.size(); i++)
      {
      validFaces[fArray[i]] = true;
      }
    }

  // loop through each face with contour points
  for (int i = 0; i < this->GetNumberOfFaces(); i++)
    {
    if (!validFaces[i])
      {
      continue;
      }

    vtkstd::vector<vtkIdType> vArray = 
      fvMap.find(static_cast<vtkIdType>(i))->second;

    
    // save contour points on this face in an array    
    vtkstd::vector<vtkIdType> polyVtxArray;
    for (size_t j = 0; j < vArray.size(); j++)
      {
      if (validPoints[vArray[j]])
        {
        polyVtxArray.push_back(vArray[j]);
        }
      }

    // skip line connection if there is only one point
    if (polyVtxArray.size() < 2)
      {
      continue;
      }

    for (size_t j = 0; j < polyVtxArray.size() - 1; j++)
      {
      if (polyEdgeTable->IsEdge(polyVtxArray[j], polyVtxArray[j+1]) == (-1))
        {
        polyEdgeTable->InsertEdge(polyVtxArray[j], polyVtxArray[j+1]);
        }
      }
    
    // for more than two points, close it
    if (polyVtxArray.size() > 2)
      {
      if (polyEdgeTable->IsEdge(
            polyVtxArray[polyVtxArray.size()], polyVtxArray[0]) == (-1))
        {
        polyEdgeTable->InsertEdge(
          polyVtxArray[polyVtxArray.size()], polyVtxArray[0]);
        }
      }
    }

  delete [] validPoints;
  delete [] validFaces;
  
  // Check if all contour points are 2-connected. If so, the contour is a simple
  // one. Otherwise, the contour may have a topology different than a plane 
  // (such as self-itersecting, closed in 3D, edge face, etc.
  // ===================================================================
  // ========= currently these special cases are not handled ===========
  // ===================================================================
  
  int *numPointEdges = new int [numberOfAllPoints];
  for (int i = 0; i < numberOfAllPoints; i++)
    {
    numPointEdges[i] = 0;
    }
  
  polyEdgeTable->InitTraversal();
  while (polyEdgeTable->GetNextEdge(p0, p1, ptr))
    {
    numPointEdges[p0] += 1;
    numPointEdges[p1] += 1;
    }

  for (int i = 0; i < numberOfAllPoints; i++)
    {
    if (numPointEdges[i] && numPointEdges[i] != 2)
      {
      vtkErrorMacro("The contour are not 2-connected. This special case is "
        "not handled currently. Contouring aborted.");
      delete [] numPointEdges;
      return;
      }
    }  

  delete [] numPointEdges;

  // finally, construct the polys
  while (!cpSet.empty())
    {
    vtkstd::vector<vtkIdType> cpLoop;

    cpSetIt = cpSet.begin();
    pId = *cpSetIt;
    cpSet.erase(cpSetIt);
    cpLoop.push_back(pId);
    
    vtkIdType startPId = pId;
    vtkIdType prevPId = -1;
    
    // continue to find the next contour point that shares an edge with the 
    // current one, until reaches the start point.
    while (!cpSet.empty())
      {
      bool foundNewPoint = false;
      polyEdgeTable->InitTraversal();
      while (polyEdgeTable->GetNextEdge(p0, p1, ptr))
        {
        if (p0 == pId && p1 != prevPId && p1 != startPId)
          {
          prevPId = pId;
          pId = p1;
          foundNewPoint = true;
          break;
          }
        if (p1 == pId && p0 != prevPId && p0 != startPId)
          {
          prevPId = pId;
          pId = p0;
          foundNewPoint = true;
          break;
          }
        }
      
      if (foundNewPoint)
        {
        cpLoop.push_back(pId);
        cpSet.erase(cpSet.find(pId));
        }
      }      
    
    // store the point in the array into polys
    polys->InsertNextCell(static_cast<vtkIdType>(cpLoop.size()), &(cpLoop[0]));
    }

  locator->InitPointInsertion(merge->GetPoints(), merge->GetBounds(), 
    merge->GetPoints()->GetNumberOfPoints());
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
