/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvexPointSet.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConvexPointSet.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"
#include "vtkOrderedTriangulator.h"
#include "vtkTetra.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkConvexPointSet, "1.7");
vtkStandardNewMacro(vtkConvexPointSet);

// Construct the hexahedron with eight points.
vtkConvexPointSet::vtkConvexPointSet()
{
  this->Tetra = vtkTetra::New();
  this->TetraIds = vtkIdList::New();
  this->TetraPoints = vtkPoints::New();
  this->TetraScalars = vtkFloatArray::New();
  this->TetraScalars->SetNumberOfTuples(4);
  this->BoundaryTris = vtkCellArray::New();
  this->BoundaryTris->Allocate(100);
  this->Triangle = vtkTriangle::New();
}

vtkConvexPointSet::~vtkConvexPointSet()
{
  this->Tetra->Delete();
  this->TetraIds->Delete();
  this->TetraPoints->Delete();
  this->TetraScalars->Delete();
  this->BoundaryTris->Delete();
  this->Triangle->Delete();
}

vtkCell *vtkConvexPointSet::MakeObject()
{
  vtkCell *cell = vtkConvexPointSet::New();
  cell->DeepCopy(this);
  return cell;
}


int vtkConvexPointSet::GetNumberOfFaces()
{
  // Initialize
  vtkIdType numPts=this->GetNumberOfPoints();
  if ( numPts < 1 ) return 0;

  // Triangulate cell and get boundary faces
  this->Triangulate(0, this->TetraIds,this->TetraPoints);
  this->BoundaryTris->Reset();
  this->Triangulator->AddTriangles(this->BoundaryTris);
  
  return this->BoundaryTris->GetNumberOfCells();
}

vtkCell *vtkConvexPointSet::GetFace(int faceId)
{
  int numCells = this->BoundaryTris->GetNumberOfCells();
  if ( faceId < 0 || faceId >=numCells ) {return NULL;}

  vtkIdType *cells = this->BoundaryTris->GetPointer();

  // Each triangle has three points plus number of points
  vtkIdType *cptr = cells + 4*faceId;
  for (int i=0; i<3; i++)
    {
    this->Triangle->PointIds->SetId(i,this->PointIds->GetId(cptr[i+1]));
    this->Triangle->Points->SetPoint(i,this->Points->GetPoint(cptr[i+1]));
    }

  return this->Triangle;
}

int vtkConvexPointSet::Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts)
{
  int numPts=this->GetNumberOfPoints();
  int i;
  float *xPtr;
  vtkIdType ptId;

  // Initialize
  ptIds->Reset();
  pts->Reset();
  if ( numPts < 1 ) return 0;
  
  // Create a triangulator if necessary.
  if ( ! this->Triangulator )
    {
    this->Triangulator = vtkOrderedTriangulator::New();
    this->Triangulator->PreSortedOff();
    }

  // Initialize Delaunay insertion process.
  // No more than numPts points can be inserted.
  this->Triangulator->InitTriangulation(this->GetBounds(), numPts);

  // Inject cell points into triangulation. Recall that the PreSortedOff() 
  // flag was set which means that the triangulator will order the points 
  // according to point id. We insert points with id == the index into the
  // vtkConvexPointSet::PointIds and Points; but sort on the global point
  // id.
  for (i=0; i<numPts; i++)
    {
    ptId = this->PointIds->GetId(i);
    xPtr = this->Points->GetPoint(i);
    this->Triangulator->InsertPoint(i, ptId, xPtr, 0);
    }//for all points
  
  // triangulate the points
  this->Triangulator->Triangulate();

  // Add the triangulation to the mesh
  this->Triangulator->AddTetras(0,ptIds,pts);
  
  return 1;
}

  
void vtkConvexPointSet::Contour(float value, vtkDataArray *cellScalars, 
                                vtkPointLocator *locator,
                                vtkCellArray *verts, vtkCellArray *lines, 
                                vtkCellArray *polys, 
                                vtkPointData *inPd, vtkPointData *outPd,
                                vtkCellData *inCd, vtkIdType cellId,
                                vtkCellData *outCd)
{
  // Initialize
  vtkIdType numPts=this->GetNumberOfPoints();
  if ( numPts < 1 ) return;

  // Triangulate with cell intersection points
  this->Triangulate(0, this->TetraIds,this->TetraPoints);
  
  // For each tetra, contour it
  int i, j;
  vtkIdType ptId;
  vtkDataArray *localScalars = inPd->GetScalars();
  int numTets = this->TetraIds->GetNumberOfIds() / 4;
  for (i=0; i<numTets; i++)
    {
    for (j=0; j<4; j++)
      {
      ptId = this->PointIds->GetId(this->TetraIds->GetId(4*i+j));
      this->Tetra->PointIds->SetId(j,ptId);
      this->Tetra->Points->SetPoint(j,this->TetraPoints->GetPoint(4*i+j));
      this->TetraScalars->SetValue(j,localScalars->GetTuple1(ptId));
      }
    this->Tetra->Contour(value,this->TetraScalars,locator,verts,lines,polys,
                         inPd,outPd,inCd,cellId,outCd);
    }
}
    


void vtkConvexPointSet::Clip(float value, 
                             vtkDataArray *vtkNotUsed(cellScalars), 
                             vtkPointLocator *locator, vtkCellArray *tets,
                             vtkPointData *inPD, vtkPointData *outPD,
                             vtkCellData *inCD, vtkIdType cellId,
                             vtkCellData *outCD, int insideOut)
{
  // Initialize
  vtkIdType numPts=this->GetNumberOfPoints();
  if ( numPts < 1 ) return;

  // Triangulate with cell intersection points
  this->Triangulate(0, this->TetraIds,this->TetraPoints);
  
  // For each tetra, contour it
  int i, j;
  vtkIdType ptId;
  vtkDataArray *localScalars = inPD->GetScalars();
  int numTets = this->TetraIds->GetNumberOfIds() / 4;
  for (i=0; i<numTets; i++)
    {
    for (j=0; j<4; j++)
      {
      ptId = this->PointIds->GetId(this->TetraIds->GetId(4*i+j));
      this->Tetra->PointIds->SetId(j,ptId);
      this->Tetra->Points->SetPoint(j,this->TetraPoints->GetPoint(4*i+j));
      this->TetraScalars->SetValue(j,localScalars->GetTuple1(ptId));
      }
    this->Tetra->Clip(value,this->TetraScalars,locator,tets,inPD,outPD,inCD,
                      cellId, outCD, insideOut);
    }
}

int vtkConvexPointSet::CellBoundary(int subId, float pcoords[3], 
                                    vtkIdList *pts)
{
  return 0;
}

int vtkConvexPointSet::EvaluatePosition(float x[3], float* closestPoint,
                                        int& subId, float pcoords[3],
                                        float& dist2, float *weights)
{
  return 0;
}

void vtkConvexPointSet::EvaluateLocation(int& subId, float pcoords[3], 
                                         float x[3], float *weights)
{
}

int vtkConvexPointSet::IntersectWithLine(float p1[3], float p2[3], float tol, 
                                         float& t, float x[3], 
                                         float pcoords[3], int& subId)
{
  return 0;
}

void vtkConvexPointSet::Derivatives(int subId, float pcoords[3], 
                                    float *values, int dim, float *derivs)
{
}



