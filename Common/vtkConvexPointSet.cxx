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
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkConvexPointSet, "1.4");
vtkStandardNewMacro(vtkConvexPointSet);

// Construct the hexahedron with eight points.
vtkConvexPointSet::vtkConvexPointSet()
{
  this->Tetras = vtkCellArray::New();
  this->Tetra = vtkTetra::New();
}

vtkConvexPointSet::~vtkConvexPointSet()
{
  this->Tetras->Delete();
  this->Tetra->Delete();
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
  vtkIdType npts, *cpts;

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

  // Initialize Delaunay insertion process with voxel triangulation.
  // No more than (numPts + numEdges) points can be inserted.
  this->Triangulator->InitTriangulation(this->GetBounds(), numPts);

  // Inject cell points into triangulation. Recall that the PreSortedOff() 
  // flag was set which means that the triangulator will order the points 
  // according to point id.
  for (i=0; i<numPts; i++)
    {
    ptId = this->PointIds->GetId(i);
    xPtr = this->Points->GetPoint(i);
    this->Triangulator->InsertPoint(ptId, xPtr, 0);
    }//for eight voxel corner points
  
  // triangulate the points
  this->Triangulator->Triangulate();

  // Add the triangulation to the mesh
  this->Tetras->Reset();
  this->Triangulator->AddTetras(0,this->Tetras);
  
  // Copy into expected output structures
  for ( this->Tetras->InitTraversal(); this->Tetras->GetNextCell(npts,cpts); )
    {
    for (i=0; i<4; i++)
      {
      ptIds->InsertNextId(cpts[i]);
      pts->InsertNextPoint(this->Points->GetPoint(cpts[i]));
      }
    }
  
  return 1;
}

  
void vtkConvexPointSet::OrderedTriangulate(vtkIdType numPts, 
                                           float value,
                                           vtkDataArray *cellScalars,
                                           vtkPointLocator *locator,
                                           vtkPointData *inPD, 
                                           vtkPointData *outPD,
                                           int insideOut)
{
  vtkIdType ptId, id;
  int i;
  float s1;
  int type;
  float *xPtr;

  // allocate scratch memory
  int *internalId = new int [numPts];

  // Create a triangulator if necessary.
  if ( ! this->Triangulator )
    {
    this->Triangulator = vtkOrderedTriangulator::New();
    this->Triangulator->PreSortedOff();
    }

  // Initialize Delaunay insertion process with voxel triangulation.    

  // No more than (numPts + numEdges) points can be inserted.
  this->Triangulator->InitTriangulation(this->GetBounds(), numPts);

  // Inject cell points into triangulation. Recall that the PreSortedOff() 
  // flag was set which means that the triangulator will order the points 
  // according to point id.
  for (i=0; i<numPts; i++)
    {
    ptId = this->PointIds->GetId(i);
      
    // Currently all points are injected because of the possibility 
    // of intersection point merging.
    s1 = cellScalars->GetComponent(i,0);
    if ( (s1 >= value && !insideOut) || (s1 < value && insideOut) )
      {
      type = 0; //inside
      }
    else
      {
      type = 4; //no insert, but its type might change later
      }

    xPtr = this->Points->GetPoint(i);
    if ( locator->InsertUniquePoint(xPtr, id) )    

      {
      outPD->CopyData(inPD,ptId, id);
      }
    internalId[i] = this->Triangulator->InsertPoint(id, xPtr, type);
    }//for eight voxel corner points
  
  // triangulate the points
  this->Triangulator->Triangulate();

  delete [] internalId;
}

void vtkConvexPointSet::Contour(float value, vtkDataArray *cellScalars, 
                                vtkPointLocator *locator,
                                vtkCellArray *vtkNotUsed(verts), 
                                vtkCellArray *vtkNotUsed(lines), 
                                vtkCellArray *polys, 
                                vtkPointData *inPd, vtkPointData *outPd,
                                vtkCellData *inCd, vtkIdType cellId,
                                vtkCellData *outCd)
{
  // Initialize
  vtkIdType numPts=this->GetNumberOfPoints();
  if ( numPts < 1 ) return;

  // Triangulate with cell intersection points
  this->OrderedTriangulate(numPts, value, cellScalars, locator, 
                           inPd, outPd, 0);
  
  // Add the surface triangles to the mesh
  this->Triangulator->AddTriangles(polys);
}
    


void vtkConvexPointSet::Clip(float value, vtkDataArray *cellScalars, 
                             vtkPointLocator *locator, vtkCellArray *tets,
                             vtkPointData *inPD, vtkPointData *outPD,
                             vtkCellData *vtkNotUsed(inCD),
                             vtkIdType vtkNotUsed(cellId),
                             vtkCellData *vtkNotUsed(outCD), int insideOut)
{
  // Initialize
  vtkIdType numPts=this->GetNumberOfPoints();
  if ( numPts < 1 ) return;

  // Triangulate with cell intersection points
  this->OrderedTriangulate(numPts, value, cellScalars, locator, 
                           inPD, outPD, insideOut);
  
  // Add the tetrahedra to the mesh
  this->Triangulator->AddTetras(0,tets);
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



