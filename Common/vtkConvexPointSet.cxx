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
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkConvexPointSet, "1.5");
vtkStandardNewMacro(vtkConvexPointSet);

// Construct the hexahedron with eight points.
vtkConvexPointSet::vtkConvexPointSet()
{
  this->Tetra = vtkTetra::New();
  this->TriIds = vtkIdList::New();
  this->TriPoints = vtkPoints::New();
  this->TriScalars = vtkFloatArray::New();
  this->TriScalars->SetNumberOfTuples(4);
  
}

vtkConvexPointSet::~vtkConvexPointSet()
{
  this->Tetra->Delete();
  this->TriIds->Delete();
  this->TriPoints->Delete();
  this->TriScalars->Delete();
}

vtkCell *vtkConvexPointSet::MakeObject()
{
  vtkCell *cell = vtkConvexPointSet::New();
  cell->DeepCopy(this);
  return cell;
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
  // according to point id.
  for (i=0; i<numPts; i++)
    {
    ptId = this->PointIds->GetId(i);
    xPtr = this->Points->GetPoint(i);
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
  this->Triangulate(0, this->TriIds,this->TriPoints);
  
  // For each tetra, contour it
  int i, j;
  vtkIdType ptId;
  vtkDataArray *localScalars = inPd->GetScalars();
  int numTets = this->TriIds->GetNumberOfIds() / 4;
  for (i=0; i<numTets; i++)
    {
    for (j=0; j<4; j++)
      {
      ptId = this->TriIds->GetId(4*i+j);
      this->Tetra->PointIds->SetId(j,ptId);
      this->Tetra->Points->SetPoint(j,this->TriPoints->GetPoint(4*i+j));
      this->TriScalars->SetValue(j,localScalars->GetTuple1(ptId));
      }
    this->Tetra->Contour(value,this->TriScalars,locator,verts,lines,polys,
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
  this->Triangulate(0, this->TriIds,this->TriPoints);
  
  // For each tetra, contour it
  int i, j;
  vtkIdType ptId;
  vtkDataArray *localScalars = inPD->GetScalars();
  int numTets = this->TriIds->GetNumberOfIds() / 4;
  for (i=0; i<numTets; i++)
    {
    for (j=0; j<4; j++)
      {
      ptId = this->TriIds->GetId(4*i+j);
      this->Tetra->PointIds->SetId(j,ptId);
      this->Tetra->Points->SetPoint(j,this->TriPoints->GetPoint(4*i+j));
      this->TriScalars->SetValue(j,localScalars->GetTuple1(ptId));
      }
    this->Tetra->Clip(value,this->TriScalars,locator,tets,inPD,outPD,inCD,
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



