/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyhedron.cxx

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
#include "vtkPointLocator.h"
#include "vtkMeanValueCoordinatesInterpolator.h"
#include "vtkSmartPointer.h"
#include "vtkMergePoints.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"

#include <vtkstd/set>
#include <vtkstd/list>
#include <limits>

vtkStandardNewMacro(vtkPolyhedron);

// Special typedef for point id map
struct vtkPointIdMap : public vtkstd::map<vtkIdType,vtkIdType> {};
typedef vtkstd::map<vtkIdType,vtkIdType*>::iterator PointIdMapIterator;

// Special class for iterating through polyhedron faces
//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
namespace
{
// insert new id element in between two existing adjacent id elements.
// this is a convenient function. no check whether the input elements 
// exist in the vector. no check for element adjacency.
int InsertNewIdToIdVector(vtkPolyhedron::IdVectorType & idVector, 
                          vtkIdType id, vtkIdType id0, vtkIdType id1)
{
  if (idVector.size() < 2)
    {
    return 0;
    }
  
  size_t num = idVector.size();
  if ((idVector[0] == id0 && idVector[num-1] == id1)
    ||(idVector[0] == id1 && idVector[num-1] == id0))
    {
    idVector.push_back(id);
    return 1;
    }
  
  vtkPolyhedron::IdVectorType::iterator iter = idVector.begin();
  for (; iter != idVector.end(); ++iter)
    {
    if (*iter == id0 || *iter == id1)
      {
      ++iter;
      idVector.insert(iter, id);
      return 1;
      }
    }
  
  return 0;
}

// Convinient function used by clip. The id is the vector index of the positive 
// point, id0 is the vector index of the start point, and id1 is the vector index
// of the end point.
//----------------------------------------------------------------------------
int EraseSegmentFromIdVector(vtkPolyhedron::IdVectorType & idVector, 
                             vtkIdType id, vtkIdType id0, vtkIdType id1)
{
  // three possible cases
  // first case: 0 -- id0 -- id -- id1 -- size-1
  if (id0 < id && id < id1)
    {
    idVector.erase(idVector.begin() + id0 + 1, idVector.begin() + id1);
    }
  // second case: 0 -- id1 -- id0 -- id -- size-1
  // third case: 0 -- id -- id1 -- id0 -- size-1
  else if (id1 < id0 && (id0 < id || id < id1))
    {
    idVector.erase(idVector.begin() + id0 + 1, idVector.end());
    idVector.erase(idVector.begin(), idVector.begin() + id1);
    }
  else
    {
    // we should never get here.
    return 0;
    }
  return 1;
}

// convert the point ids from map.first to map.second
//----------------------------------------------------------------------------
int ConvertPointIds(vtkIdType npts, vtkIdType * pts, 
                    vtkPolyhedron::IdToIdMapType & map)
{
  for (vtkIdType i = 0; i < npts; i++)
    {
    vtkPolyhedron::IdToIdMapType::iterator iter = map.find(pts[i]);
    if (iter == map.end())
      {
      return 0;
      }
    pts[i] = iter->second;
    }
  return 1;
}

//----------------------------------------------------------------------------
// Use eigenvalues to determine the dimension of the input contour points.
// This chunk of code is mostly copied from vtkOBBTree::ComputeOBB()
// Function return 0 if input is a single point, 1 if co-linear, 
// 2 if co-planar, 3 if 3D
int CheckContourDimensions(vtkPoints* points, vtkIdType npts, vtkIdType * ptIds)
{
  static const double eigenvalueRatioThresh = 0.001;
  
  if (npts < 3)
    {
    return npts - 1;
    }
  
  vtkIdType i, j;
  double x[3], mean[3], xp[3], *v[3], v0[3], v1[3], v2[3];
  double *a[3], a0[3], a1[3], a2[3], eigValue[3];

  // Compute mean
  mean[0] = mean[1] = mean[2] = 0.0;
  for (i=0; i < npts; i++ )
    {
    points->GetPoint(ptIds[i], x);
    mean[0] += x[0];
    mean[1] += x[1];
    mean[2] += x[2];
    }
  for (i=0; i < 3; i++)
    {
    mean[i] /= npts;
    }

  // Compute covariance matrix
  a[0] = a0; a[1] = a1; a[2] = a2; 
  for (i=0; i < 3; i++)
    {
    a0[i] = a1[i] = a2[i] = 0.0;
    }

  for (j = 0; j < npts; j++ )
    {
    points->GetPoint(ptIds[j], x);
    xp[0] = x[0] - mean[0]; xp[1] = x[1] - mean[1]; xp[2] = x[2] - mean[2];
    for (i = 0; i < 3; i++)
      {
      a0[i] += xp[0] * xp[i];
      a1[i] += xp[1] * xp[i];
      a2[i] += xp[2] * xp[i];
      }
    }//for all points

  for (i=0; i < 3; i++)
    {
    a0[i] /= npts;
    a1[i] /= npts;
    a2[i] /= npts;
    }

  // Extract axes (i.e., eigenvectors) from covariance matrix. 
  v[0] = v0; v[1] = v1; v[2] = v2; 
  vtkMath::Jacobi(a,eigValue,v);

  int ret = 3;
  
  if ((eigValue[2] / eigValue[0]) < eigenvalueRatioThresh)
    {
    ret--;
    }
  if ((eigValue[1] / eigValue[0]) < eigenvalueRatioThresh)
    {
    ret--;
    }
  
  return ret;
}


//----------------------------------------------------------------------------
// Note: the triangulation results are inserted into the input cellArray, which
// does not need to be empty.
void Triangulate3DContour(vtkIdType npts, vtkIdType * pts, 
                          vtkCellArray *cellArray)
{
  vtkIdType start = 0;
  vtkIdType end = npts-1;
  vtkIdType ids[3];
  
  while (start < end)
    {
    ids[0] = pts[start++];
    ids[1] = pts[start];
    ids[2] = pts[end];
    cellArray->InsertNextCell(3, ids);

    if (start >= end - 1)
      {
      return;
      }
    
    ids[0] = pts[end];
    ids[1] = pts[start];
    ids[2] = pts[--end];
    cellArray->InsertNextCell(3, ids);
    }
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

vtkPolyData* vtkPolyhedron::GetPolyData()
{
  if (!this->PolyDataConstructed)
    {
    this->ConstructPolyData();
    }
  
  return this->PolyData;
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

  if (!faces)
    {
    return;
    }
    
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
  if (this->IsInside(x, std::numeric_limits<double>::infinity()))
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
  for (vtkIdType i = 0; i < this->GetNumberOfPoints(); i++)
    {
    this->GetPoints()->GetPoint(i, point);
    triangulator->InsertPoint(i, point, point, 0);
    }
  triangulator->Triangulate();
  
  triangulator->AddTetras(0, ptIds, pts);
  
  // convert to global Ids
  vtkIdType* ids = ptIds->GetPointer(0);
  for (vtkIdType i = 0; i < ptIds->GetNumberOfIds(); i++)
    {
    ids[i] = this->PointIds->GetId(ids[i]);
    }
    
  return 1;
}

//----------------------------------------------------------------------------
// NOTE: inScalars are in canonoical id space. while inPd are in global id space.
void vtkPolyhedron::InternalContour(double value,
                            vtkIncrementalPointLocator *locator,
                            vtkDataArray *inScalars,
                            vtkDataArray *outScalars,
                            vtkPointData *inPd, 
                            vtkPointData *outPd,
                            vtkCellArray *contourPolys,
                            IdToIdVectorMapType & faceToPointsMap,
                            IdToIdVectorMapType & pointToFacesMap,
                            IdToIdMapType & pointIdMap)
{
  double x0[3], x1[3], x[3];
  double v0, v1, t;

  vtkIdType p0, p1, flag, pid, fid, outPid, globalP0, globalP1;
  void * ptr = NULL;

  // initialization
  this->GenerateEdges();
  this->GenerateFaces();
  this->ConstructPolyData();
  this->ComputeBounds();

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->DeepCopy(this->Points);
  vtkSmartPointer<vtkMergePoints> merge = vtkSmartPointer<vtkMergePoints>::New();
  merge->InitPointInsertion(points, this->Bounds);
  for (int i = 0; i < points->GetNumberOfPoints(); i++)
    {
    merge->InsertUniquePoint(points->GetPoint(i), pid);
    }

  for (vtkIdType i = 0; i < inScalars->GetNumberOfTuples(); i++)
    {
    outScalars->InsertNextTuple1(inScalars->GetTuple1(i));
    }
  
  pointToFacesMap.clear();
  faceToPointsMap.clear();
  pointIdMap.clear();
  
  IdToIdVectorMapIteratorType vfMapIt, vfMapIt0, vfMapIt1;
  IdToIdVectorMapIteratorType fvMapIt;
  
  // loop through all faces to construct PointToFacesMap and FaceToPointsMap
  vtkPolyhedronFaceIterator 
    faceIter(this->GetNumberOfFaces(), this->Faces->GetPointer(1));
  while (faceIter.Id < faceIter.NumberOfPolygons)
    {
    // the rest code of this function assumes that a face contains at least 
    // three vertices. return if find a single-vertex or double-vertex face.
    if (faceIter.CurrentPolygonSize < 3)
      {
      vtkErrorMacro("Find a face with " << faceIter.CurrentPolygonSize <<
        " vertices. Contouring aborted due to this degenrate case.");
      return;      
      }

    fid = faceIter.Id;
    IdVectorType vVector;
    for (vtkIdType i = 0; i < faceIter.CurrentPolygonSize; i++)
      {
      pid = faceIter.Current[i];
      vfMapIt = pointToFacesMap.find(pid);
      if (vfMapIt != pointToFacesMap.end())
        {
        vfMapIt->second.push_back(fid);
        }
      else
        {
        IdVectorType fVector;
        fVector.push_back(fid);
        pointToFacesMap.insert(IdToIdVectorPairType(pid, fVector));
        }
      vVector.push_back(pid);
      }
    
    faceToPointsMap.insert(IdToIdVectorPairType(fid, vVector));
    ++faceIter;
    }

  const double eps = 0.0000000001;

  // loop through all edges to find contour points and store them in the point 
  // locator. if the contour points are new (not overlap with any of original 
  // vertex), update PointToFacesMap and FaceToPointsMap.
  
  IdSetType cpSet; // contour point set
  this->EdgeTable->InitTraversal();
  while (this->EdgeTable->GetNextEdge(p0, p1, ptr))
    {
    v0 = static_cast<double>(inScalars->GetComponent(p0,0));
    v1 = static_cast<double>(inScalars->GetComponent(p1,0));

    globalP0 = this->PointIds->GetId(p0);
    globalP1 = this->PointIds->GetId(p1);

    points->GetPoint(p0, x0);
    points->GetPoint(p1, x1);
    
    if (v1-value > eps && v0-value > eps)
      {
      continue;
      }
    if (v1-value < -eps && v0-value < -eps)
      {
      continue;
      }
    
    flag = 0;
    if (v0 - value < eps && v0 - value > -eps)
      {
      cpSet.insert(p0);
      if (locator->InsertUniquePoint(x0, outPid))
        {
        outPd->CopyData(inPd, globalP0, outPid);
        }
      pointIdMap.insert(IdToIdPairType(p0, outPid));
      flag = 1;
      }
    if (v1 - value < eps && v1 - value > -eps)
      {
      cpSet.insert(p1);
      if (locator->InsertUniquePoint(x1, outPid))
        {
        outPd->CopyData(inPd, globalP1, outPid);
        }
      pointIdMap.insert(IdToIdPairType(p1, outPid));
      flag = 1;
      }
    
    if (flag)
      {
      continue;
      }
    
    t = (value - v0)/(v1 - v0);
    x[0] = (1 - t) * x0[0] + t * x1[0];
    x[1] = (1 - t) * x0[1] + t * x1[1];
    x[2] = (1 - t) * x0[2] + t * x1[2];

    if (merge->InsertUniquePoint(x, pid))
      {
      // find adjacent faces for the new point
      vfMapIt0 = pointToFacesMap.find(p0);
      vfMapIt1 = pointToFacesMap.find(p1);
      if (vfMapIt0 == pointToFacesMap.end() || vfMapIt1 == pointToFacesMap.end())
        {
        vtkErrorMacro("Cannot locate adjacent faces of a vertex. We should never "
          "get here. Contouring continues but may generate wrong result.");
        continue;
        }
      
      IdVectorType fVector0 = vfMapIt0->second;
      IdVectorType fVector1 = vfMapIt1->second;
      vtkstd::set<vtkIdType> fSet;
      for (size_t i = 0; i < fVector0.size(); i++)
        {
        for (size_t j = 0; j < fVector1.size(); j++)
          {
          if (fVector0[i] == fVector1[j])
            {
            fSet.insert(fVector0[i]);
            }
          }
        }
      if (fSet.empty())
        {
        vtkErrorMacro("Cannot locate adjacent faces of a contour point. We should "
          "never get here. Contouring continues but may generate wrong result.");
        continue;
        }

      // update PointToFacesMap: add adjacent faces to the new point
      IdVectorType fVector;
      vtkstd::set<vtkIdType>::iterator fSetIt;
      for (fSetIt = fSet.begin(); fSetIt != fSet.end(); ++fSetIt)
        {
        fVector.push_back(*fSetIt);
        }
      pointToFacesMap.insert(IdToIdVectorPairType(pid, fVector));

      // update FaceToPointsMap: insert the new point to the adjacent faces
      for (fSetIt = fSet.begin(); fSetIt != fSet.end(); ++fSetIt)
        {
        fvMapIt = faceToPointsMap.find(*fSetIt);
        InsertNewIdToIdVector(fvMapIt->second, pid, p0, p1);
        }

      // Maintain point data. only add to locator when it has never been added
      // as contour point of previous processed cells.
      if (locator->InsertUniquePoint(x, outPid) && outPd)
        {
        outPd->InterpolateEdge(inPd,outPid,globalP0,globalP1,t);
        }

      // A point unique to merge may not be unique to locator, since it may have 
      // been inserted to locator as contour point of previous processed cells.
      if (outScalars)
        {
        outScalars->InsertTuple1(pid, value);
        }

      pointIdMap.insert(IdToIdPairType(pid, outPid));
      }
      
    cpSet.insert(pid);
    }
  
  int numberOfAllPoints = merge->GetPoints()->GetNumberOfPoints();
  
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
  IdVectorType fVector;
  for (cpSetIt = cpSet.begin(); cpSetIt != cpSet.end(); ++cpSetIt)
    {
    validPoints[*cpSetIt] = true;
    
    fVector = pointToFacesMap.find(*cpSetIt)->second;
    for (size_t i = 0; i < fVector.size(); i++)
      {
      validFaces[fVector[i]] = true;
      }
    }

  // loop through each face with contour points
  for (int i = 0; i < this->GetNumberOfFaces(); i++)
    {
    if (!validFaces[i])
      {
      continue;
      }

    IdVectorType vVector = 
      faceToPointsMap.find(static_cast<vtkIdType>(i))->second;

    
    // save contour points on this face in an vector    
    IdVectorType polyVtxVector;
    for (size_t j = 0; j < vVector.size(); j++)
      {
      if (validPoints[vVector[j]])
        {
        polyVtxVector.push_back(vVector[j]);
        }
      }

    // skip line connection if there is only one point
    if (polyVtxVector.size() < 2)
      {
      continue;
      }

    for (size_t j = 0; j < polyVtxVector.size() - 1; j++)
      {
      if (polyEdgeTable->IsEdge(polyVtxVector[j], polyVtxVector[j+1]) == (-1))
        {
        polyEdgeTable->InsertEdge(polyVtxVector[j], polyVtxVector[j+1]);
        }
      }
    
    // for more than two points, close it. Note that we need to check degenerate
    // case where all points in polyVtxVector lie on the same line. Otherwise,
    // we will generate a line polygon.
    if (polyVtxVector.size() > 2 && polyEdgeTable->IsEdge(
        polyVtxVector[polyVtxVector.size()-1], polyVtxVector[0]) == (-1))
      {
      int ret = CheckContourDimensions(merge->GetPoints(), 
                                       polyVtxVector.size(), &polyVtxVector[0]);
      if (ret > 1)
        {
        polyEdgeTable->InsertEdge(
          polyVtxVector[polyVtxVector.size()-1], polyVtxVector[0]);
        }
      }

    } // end of face loop

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
    IdVectorType cpLoop;

    cpSetIt = cpSet.begin();
    pid = *cpSetIt;
    cpSet.erase(cpSetIt);
    cpLoop.push_back(pid);
    
    vtkIdType startPId = pid;
    vtkIdType prevPId = -1;
    
    // continue to find the next contour point that shares an edge with the 
    // current one, until reaches the start point.
    while (!cpSet.empty())
      {
      bool foundNewPoint = false;
      polyEdgeTable->InitTraversal();
      while (polyEdgeTable->GetNextEdge(p0, p1, ptr))
        {
        if (p0 == pid && p1 != prevPId && p1 != startPId)
          {
          prevPId = pid;
          pid = p1;
          foundNewPoint = true;
          break;
          }
        if (p1 == pid && p0 != prevPId && p0 != startPId)
          {
          prevPId = pid;
          pid = p0;
          foundNewPoint = true;
          break;
          }
        }
      
      if (foundNewPoint)
        {
        cpLoop.push_back(pid);
        cpSet.erase(cpSet.find(pid));
        }
      }

    vtkIdType npts = static_cast<vtkIdType>(cpLoop.size());
    vtkIdType *pts = &(cpLoop[0]);

    // check the dimensionality of the contour
    int ret = CheckContourDimensions(merge->GetPoints(), npts, pts);
    
    if (ret <= 1) // skip single point or co-linear points
      {
      }
    else if (ret == 2) // planar polygon, add directly
      {
      contourPolys->InsertNextCell(npts, pts);
      }
    else  // 3D points, need to triangulate the original polygon
      {
      Triangulate3DContour(npts, pts, contourPolys); 
      }
    }
}


//----------------------------------------------------------------------------
void vtkPolyhedron::Contour(double value,
                            vtkDataArray *pointScalars,
                            vtkIncrementalPointLocator *locator,
                            vtkCellArray *verts,
                            vtkCellArray *lines,
                            vtkCellArray *polys,
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId,
                            vtkCellData *outCd)
{
  vtkIdType offset = 0;
  if (verts)
    {
    offset += verts->GetNumberOfCells();
    }
  if (lines)
    {
    offset += lines->GetNumberOfCells();
    }

  IdToIdVectorMapType faceToPointsMap;
  IdToIdVectorMapType pointToFacesMap;
  IdToIdMapType       pointIdMap;

  vtkSmartPointer<vtkCellArray> contourPolys =
    vtkSmartPointer<vtkCellArray>::New();
  
  this->InternalContour(value, locator, pointScalars, NULL, 
    inPd, outPd, contourPolys, faceToPointsMap, pointToFacesMap, pointIdMap);

  vtkIdType npts = 0;
  vtkIdType *pts = 0;
  contourPolys->InitTraversal();
  while (contourPolys->GetNextCell(npts, pts))
    {
    if (!ConvertPointIds(npts, pts, pointIdMap))
      {
      vtkErrorMacro("Cannot find the id of an output point. We should never "
        "get here. Contouring aborted.");
      return;
      }
    
    vtkIdType newCellId = offset + polys->InsertNextCell(npts, pts);
    outCd->CopyData(inCd, cellId, newCellId);
    }
}


//----------------------------------------------------------------------------
void vtkPolyhedron::Clip(double value,
                         vtkDataArray *pointScalars,
                         vtkIncrementalPointLocator *locator, 
                         vtkCellArray *connectivity,
                         vtkPointData *inPd, vtkPointData *outPd,
                         vtkCellData *inCd, vtkIdType cellId,
                         vtkCellData *outCd, int insideOut)
{
  IdToIdVectorMapType faceToPointsMap;
  IdToIdVectorMapType pointToFacesMap;
  IdToIdMapType       pointIdMap;
  vtkIdType newPid, newCellId;

  vtkIdType npts = 0;
  vtkIdType *pts = 0;

  // vector to store cell connectivity
  IdVectorType cellVector;

  vtkSmartPointer<vtkDoubleArray> contourScalars = 
    vtkSmartPointer<vtkDoubleArray>::New();
  vtkSmartPointer<vtkCellArray> contourPolys =
    vtkSmartPointer<vtkCellArray>::New();

  this->InternalContour(value, locator, pointScalars, contourScalars, 
    inPd, outPd, contourPolys, faceToPointsMap, pointToFacesMap, pointIdMap);

  
  // if empty contour, the polyhedron will be all inside or all outside
  if (!contourPolys->GetNumberOfCells())
    {
    int numPositivePoints = 0;
    for (vtkIdType i = 0; i < this->GetNumberOfPoints(); i++)
      {
      double v = static_cast<double>(pointScalars->GetComponent(i,0));
      if (((v < value) && insideOut) || ((v > value) && (!insideOut)))
        {
        numPositivePoints++;
        }
      }
    
    // point or line cases
    if (numPositivePoints < 3)
      {
      return;
      }

    cellVector.push_back(this->GetNumberOfFaces());

    // loop through all faces to add them into cellVector
    vtkPolyhedronFaceIterator 
      faceIter(this->GetNumberOfFaces(), this->Faces->GetPointer(1));
    while (faceIter.Id < faceIter.NumberOfPolygons)
      {
      IdVectorType pids;
      for (vtkIdType i = 0; i < faceIter.CurrentPolygonSize; i++)
        {
        vtkIdType pid = faceIter.Current[i];
        if (locator->InsertUniquePoint(this->Points->GetPoint(pid), newPid))
          {
          vtkIdType globalPid = this->PointIds->GetId(pid);
          outPd->CopyData(inPd, globalPid, newPid);
          }
        pids.push_back(pid);
        pointIdMap.insert(IdToIdPairType(pid, newPid));
        }

      npts = static_cast<vtkIdType>(pids.size());
      pts = &(pids[0]);
      if (!ConvertPointIds(npts, pts, pointIdMap))
        {
        vtkErrorMacro("Cannot find the id of an output point. We should never "
          "get here. Clipping aborted.");
        return;
        }
      cellVector.push_back(npts);
      cellVector.insert(cellVector.end(), pts, pts+npts);
     
      ++faceIter;
      }
    
    newCellId = connectivity->InsertNextCell(
      static_cast<vtkIdType>(cellVector.size()), &(cellVector[0]));
    outCd->CopyData(inCd, cellId, newCellId);
    
    return;
    }

  // prepare visited array for all faces
  bool* visited = new bool [this->GetNumberOfFaces()];
  for (int i = 0; i < this->GetNumberOfFaces(); i++)
    {
    visited[i] = false;
    }
  
  const double eps = 0.0000000001;
  
  // Main algorithm: go through all positive points (points on the right side 
  // of the contour).  These do not include contour points.
  // For each point on the right side, find all of its adjacent faces. There 
  // maybe two types of faces, (1) faces with all positive points, or 
  // (2) faces with positive negative and contour points. For case (1), we will
  // keep the original face and add it into the result polyhedron. For case (2),
  // we will subdivide the original face, and add the subface that includes 
  // positive points into the result polyhedron.
  vtkstd::vector<IdVectorType> faces;
  IdToIdVectorMapIteratorType pfMapIt, fpMapIt;
  for (vtkIdType pid = 0; pid < this->Points->GetNumberOfPoints(); pid++)
    {
    // find if a point is a positive point
    double v = contourScalars->GetComponent(pid,0);
    if ( (insideOut && (v > value-eps)) || ((!insideOut) && (v < value+eps)) )
      {
      continue;
      }
    
    // find adjacent faces of the positive point
    pfMapIt = pointToFacesMap.find(pid);
    if (pfMapIt == pointToFacesMap.end())
      {
      vtkErrorMacro("Cannot locate adjacent faces of a positive point. We should "
        "never get here. Clipping continues but may generate wrong result.");
      continue;
      }
    IdVectorType fids = pfMapIt->second;
    
    // for each adjacent face
    for (size_t i = 0; i < fids.size(); i++)
      {
      vtkIdType fid = fids[i];
      if (visited[fid])
        {
        continue;
        }
      
      fpMapIt = faceToPointsMap.find(fid);
      if (fpMapIt == faceToPointsMap.end())
        {
        vtkErrorMacro("Cannot locate points on a face. We should "
          "never get here. Clipping continues but may generate wrong result.");
        continue;
        }
      IdVectorType pids = fpMapIt->second;
      vtkIdType numFacePoints = static_cast<vtkIdType>(pids.size());

      // locate the positive point inside the id vector.      
      vtkIdType positivePt = -1;
      for (vtkIdType j = 0; j < numFacePoints; j++)
        {
        if (pid == pids[j])
          {
          positivePt = j;
          break;
          }
        }
      
      // positive point not found: this can happen when the current face 
      // has been partially visited before, and some points have been removed from 
      // its point vector. 
      if (positivePt < 0 || positivePt >= numFacePoints)
        {
        continue;
        }

      // a new id vector to hold ids of points on new surface patch
      IdVectorType newpids;
      newpids.push_back(pid);

      // step through the ajacent points on both sides of the positive point.
      // stop when a contour point or a negative point is hit.
      bool startFound = false;
      bool endFound = false;

      vtkIdType startPt = positivePt - 1;
      vtkIdType endPt = positivePt + 1;
      for (vtkIdType k = 0; k < numFacePoints; k++)
        {
        if (startFound && endFound)
          {
          break;
          }
        
        if (!startFound)
          {
          if (startPt < 0)
            {
            startPt = numFacePoints - 1;
            }
          
          newpids.insert(newpids.begin(), pids[startPt]);
          v = contourScalars->GetComponent(pids[startPt],0);
          if ((insideOut && (v > value-eps)) || ((!insideOut) && (v < value+eps)))
            {
            startFound = true;
            if ((insideOut && (v > value+eps)) || ((!insideOut) && (v < value-eps)))
              {
              vtkWarningMacro("A positive point is directly connected to a "
                "negative point with no contour point in between. We should "
                "never get here.");
              startPt = startPt == numFacePoints-1 ? 0 : startPt++;
              newpids.erase(newpids.begin());
              }
            }
          else
            {
            startPt--;
            }                
          }

        if (!endFound)
          {
          if (endPt > numFacePoints - 1)
            {
            endPt = 0;
            }
          
          newpids.push_back(pids[endPt]);
          v = contourScalars->GetComponent(pids[endPt],0);
          if ((insideOut && (v > value-eps)) || ((!insideOut) && (v < value+eps)))
            {
            endFound = true;
            if ((insideOut && (v > value+eps)) || ((!insideOut) && (v < value-eps)))
              {
              vtkWarningMacro("A positive point is directly connected to a "
                "negative point with no contour point in between. We should "
                "never get here.");
              endPt = endPt == 0 ? numFacePoints-1 : endPt--;
              newpids.pop_back();
              }
            }
          else
            {
            endPt++;
            }                
          }
        }// end inner for loop for finding start and end points

      // if face are entirely positive, add it directly into the face list
      if (!startFound && !endFound)
        {
        visited[fid] = true;
        faces.push_back(pids);
        }
      
      // if face contain contour points
      else if (startFound && endFound)
        {
        // a point or a line
        if (newpids.size() < 3)
          {
          visited[fid] = true;
          }
        // if face only contains one contour point, this is a special case that
        // may only happen when one of the original vertex is a contour point.
        // we will add this face to the result polyhedron.
        else if (startPt == endPt)
          {
          visited[fid] = true;
          faces.push_back(pids);
          }
        // Face contain at least two contour points. In this case, we will create
        // a new face patch whose close boundary is start point -->contour point
        // --> end point --> start point. Notice that the face may contain other
        // positive points and contour points. So we will not label the face as 
        // visited. Instead, we will erase the chunk from start point to end
        // point from the point id vector of the face. So that the other part
        // can still be visited in the future.
        else
          {
          if (!EraseSegmentFromIdVector(pids, positivePt, startPt, endPt))
            {
            vtkErrorMacro("Erase segment from Id vector failed. We should never get "
              "here.");
            visited[fid] = true;
            continue;
            }
          if (pids.size()<=2) // all but two contour points are left
            {
            pids.clear();
            visited[fid] = true;
            }
          fpMapIt->second = pids;
          faces.push_back(newpids); 
          }
        }
      
      // only find start or only find end. this should never happen
      else
        {
        visited[fid] = true;
        vtkErrorMacro("We should never get here. Locating contour points failed. "
          "Clipping continues but may generate wrong result.");
        }
      } // end for each face

    } // end for_pid 

  vtkIdType numAllFaces = contourPolys->GetNumberOfCells() + 
                          static_cast<vtkIdType>(faces.size());
  cellVector.push_back(numAllFaces);
  
  // add contour faces
  contourPolys->InitTraversal();
  while (contourPolys->GetNextCell(npts, pts))
    {
    if (!ConvertPointIds(npts, pts, pointIdMap))
      {
      vtkErrorMacro("Cannot find the id of an output point. We should never "
        "get here. Clipping aborted.");
      return;
      }
    cellVector.push_back(npts);
    cellVector.insert(cellVector.end(), pts, pts+npts);
    }

  // add other faces
  for (size_t i = 0; i < faces.size(); i++)
    {
    IdVectorType pids = faces[i];
    for (size_t j = 0; j < pids.size(); j++)
      {
      vtkIdType pid = pids[j];
      vtkPolyhedron::IdToIdMapType::iterator iter = pointIdMap.find(pid);
      if (iter == pointIdMap.end()) // must be original points
        {
        if (locator->InsertUniquePoint(this->Points->GetPoint(pid), newPid))
          {
          vtkIdType globalPid = this->PointIds->GetId(pid);
          outPd->CopyData(inPd, globalPid, newPid);
          }
        pointIdMap.insert(IdToIdPairType(pid, newPid));
        }
      }
    
    npts = static_cast<vtkIdType>(pids.size());
    pts = &(pids[0]);
    if (!ConvertPointIds(npts, pts, pointIdMap))
      {
      vtkErrorMacro("Cannot find the id of an output point. We should never "
        "get here. Clipping aborted.");
      return;
      }
    cellVector.push_back(npts);
    cellVector.insert(cellVector.end(), pts, pts+npts);
    }

  newCellId = connectivity->InsertNextCell(
    static_cast<vtkIdType>(cellVector.size()), &(cellVector[0]));
  outCd->CopyData(inCd, cellId, newCellId);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::DecomposeAPolyhedronCell(vtkCellArray * polyhedronCell, 
       vtkIdType & numCellPts, vtkIdType & nCellfaces, 
       vtkCellArray * cellArray, vtkIdTypeArray * faces)
{
  vtkIdType *cellStream = 0;
  vtkIdType cellLength = 0;
  
  polyhedronCell->InitTraversal();
  polyhedronCell->GetNextCell(cellLength, cellStream);
  
  vtkPolyhedron::DecomposeAPolyhedronCell(
    cellStream, numCellPts, nCellfaces, cellArray, faces);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::DecomposeAPolyhedronCell(vtkIdType *cellStream,
       vtkIdType & numCellPts, vtkIdType & nCellFaces, 
       vtkCellArray * cellArray, vtkIdTypeArray * faces)
{
  nCellFaces = cellStream[0];
  if (nCellFaces <= 0)
    {
    return;
    }
  
  vtkPolyhedron::DecomposeAPolyhedronCell(
    nCellFaces, cellStream+1, numCellPts, cellArray, faces);
}

//----------------------------------------------------------------------------
void vtkPolyhedron::DecomposeAPolyhedronCell(vtkIdType nCellFaces,
       vtkIdType * cellStream, vtkIdType & numCellPts, 
       vtkCellArray * cellArray, vtkIdTypeArray * faces)
{
  IdSetType cellPointSet;
  IdSetType::iterator it;
  
  // insert number of faces into the face array
  faces->InsertNextValue(nCellFaces);
  
  // for each face
  for (vtkIdType fid = 0; fid < nCellFaces; fid++)
    {
    // extract all points on the same face, store them into a set
    vtkIdType npts = *cellStream++;
    faces->InsertNextValue(npts);
    for (vtkIdType i = 0; i < npts; i++)
      {
      vtkIdType pid = *cellStream++;
      faces->InsertNextValue(pid);
      cellPointSet.insert(pid);
      }
    }
  
  // standard cell connectivity array that stores the number of points plus
  // a list of point ids.
  cellArray->InsertNextCell(static_cast<int>(cellPointSet.size()));
  for (it = cellPointSet.begin(); it != cellPointSet.end(); ++it)
    {
    cellArray->InsertCellPoint(*it);
    }
  
  // the real number of points in the polyhedron cell.
  numCellPts = cellPointSet.size();
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
