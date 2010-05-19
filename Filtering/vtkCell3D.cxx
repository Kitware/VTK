/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCell3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCell3D.h"

#include "vtkOrderedTriangulator.h"
#include "vtkPointLocator.h"
#include "vtkMarchingCubesCases.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkTetra.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"


vtkCell3D::vtkCell3D()
{
  this->Triangulator = NULL;
  this->MergeTolerance = 0.01;
  this->ClipTetra = NULL;
  this->ClipScalars = NULL;
}

vtkCell3D::~vtkCell3D()
{
  if ( this->Triangulator )
    {
    this->Triangulator->Delete();
    this->Triangulator = NULL;
    }
  if ( this->ClipTetra )
    {
    this->ClipTetra->Delete();
    this->ClipTetra = NULL;
    this->ClipScalars->Delete();
    this->ClipScalars = NULL;
    }
}

void vtkCell3D::Contour(double value, vtkDataArray *cellScalars, 
                        vtkIncrementalPointLocator *locator, vtkCellArray *verts,
                        vtkCellArray *lines, vtkCellArray *polys,
                        vtkPointData *inPd, vtkPointData *outPd,
                        vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd)
{
  int numPts=this->GetNumberOfPoints();
  int numEdges=this->GetNumberOfEdges();
  int *tets, v1, v2;
  int i, j;
  int type;
  vtkIdType id, ptId;
  vtkIdType internalId[VTK_CELL_SIZE];
  double s1, s2, x[3], t, p1[3], p2[3], deltaScalar;
  
  // Create a triangulator if necessary.
  if ( ! this->Triangulator )
    {
    this->Triangulator = vtkOrderedTriangulator::New();
    this->Triangulator->PreSortedOff();
    this->Triangulator->UseTemplatesOn();
    this->ClipTetra = vtkTetra::New();
    this->ClipScalars = vtkDoubleArray::New();
    this->ClipScalars->SetNumberOfTuples(4);
    }

  // If here, the ordered triangulator is going to be used so the triangulation
  // has to be initialized.
  this->Triangulator->InitTriangulation(0.0,1.0, 0.0,1.0, 0.0,1.0,
                                        (numPts + numEdges));

  // Cells with fixed topology are triangulated with templates.
  double *p, *pPtr = this->GetParametricCoords();
  if ( this->IsPrimaryCell() )
    {
    // Some cell types support templates for interior clipping. Templates
    // are a heck of a lot faster.
    type = 0; //inside
    for (p=pPtr, i=0; i<numPts; i++, p+=3)
      {
      ptId = this->PointIds->GetId(i);
      this->Points->GetPoint(i, x);
      this->Triangulator->InsertPoint(ptId, x, p, type);
      }//for all cell points of fixed topology

    this->Triangulator->TemplateTriangulate(this->GetCellType(),
                                            numPts,numEdges);

    // Otherwise we have produced tetrahedra and now contour these using
    // the faster vtkTetra::Contour() method.
      for ( this->Triangulator->InitTetraTraversal(); 
            this->Triangulator->GetNextTetra(0,this->ClipTetra,
                                             cellScalars,this->ClipScalars);)
        {
        this->ClipTetra->Contour(value, this->ClipScalars, locator, 
                                 verts, lines, polys, inPd, outPd, inCd, 
                                 cellId, outCd);
        }
    return;
    } //if we are clipping fixed topology
  
  // If here we're left with a non-fixed topology cell (e.g. convex point set).
  // Inject cell points into triangulation. Recall that the PreSortedOff() 
  // flag was set which means that the triangulator will order the points 
  // according to point id.
  for (p=pPtr, i=0; i<numPts; i++, p+=3)
    {
    ptId = this->PointIds->GetId(i);
      
    // Currently all points are injected because of the possibility 
    // of intersection point merging.
    s1 = cellScalars->GetComponent(i,0);
    if ( (s1 >= value) || (s1 < value) )
      {
      type = 0; //inside
      }
    else
      {
      type = 4; //outside, its type might change later (nearby intersection)
      }

    this->Points->GetPoint(i, x);
    if ( locator->InsertUniquePoint(x, id) )
      {
      outPd->CopyData(inPd,ptId, id);
      }
    internalId[i] = this->Triangulator->InsertPoint(id, x, p, type);
    }//for all points
  
  // For each edge intersection point, insert into triangulation. Edge
  // intersections come from contouring value. Have to be careful of 
  // intersections near exisiting points (causes bad Delaunay behavior).
  // Intersections near existing points are collapsed to existing point.
  double pc[3], *pc1, *pc2;
  for (int edgeNum=0; edgeNum < numEdges; edgeNum++)
    {
    this->GetEdgePoints(edgeNum, tets);

    // Calculate a preferred interpolation direction.
    // Has to be done in same direction to insure coincident points are
    // merged (different interpolation direction causes pertubations).
    s1 = cellScalars->GetComponent(tets[0],0);
    s2 = cellScalars->GetComponent(tets[1],0);

    if ( (s1 <= value && s2 >= value) || (s1 >= value && s2 <= value) )
      {
      deltaScalar = s2 - s1;

      if (deltaScalar > 0)
        {
        v1 = tets[0]; v2 = tets[1];
        }
      else
        {
        v1 = tets[1]; v2 = tets[0];
        deltaScalar = -deltaScalar;
        }

      // linear interpolation
      t = ( deltaScalar == 0.0 ? 0.0 :
            (value - cellScalars->GetComponent(v1,0)) / deltaScalar );

      if ( t < this->MergeTolerance )
        {
        this->Triangulator->UpdatePointType(internalId[v1], 2);
        continue;
        }
      else if ( t > (1.0 - this->MergeTolerance) )
        {
        this->Triangulator->UpdatePointType(internalId[v2], 2);
        continue;
        }

      this->Points->GetPoint(v1, p1);
      this->Points->GetPoint(v2, p2);
      pc1 = pPtr + 3*v1;
      pc2 = pPtr + 3*v2;

      for (j=0; j<3; j++)
        {
        x[j] = p1[j] + t*(p2[j] - p1[j]);
        pc[j] = pc1[j] + t*(pc2[j] - pc1[j]);
        }
      
      // Incorporate point into output and interpolate edge data as necessary
      if ( locator->InsertUniquePoint(x, ptId) )
        {
        outPd->InterpolateEdge(inPd, ptId, this->PointIds->GetId(v1),
                               this->PointIds->GetId(v2), t);
        }

      //Insert intersection point into Delaunay triangulation
      this->Triangulator->InsertPoint(ptId,x,pc,2);

      }//if edge intersects value
    }//for all edges
  
  // triangulate the points
  this->Triangulator->Triangulate();

  // Add the triangulation to the mesh
  this->Triangulator->AddTetras(0,polys);

}

void vtkCell3D::Clip(double value, vtkDataArray *cellScalars, 
                     vtkIncrementalPointLocator *locator, vtkCellArray *tets,
                     vtkPointData *inPD, vtkPointData *outPD,
                     vtkCellData *inCD, vtkIdType cellId,
                     vtkCellData *outCD, int insideOut)
{
  vtkCell3D *cell3D = static_cast<vtkCell3D *>(this); //has to be in this method
  int numPts=this->GetNumberOfPoints();
  int numEdges=this->GetNumberOfEdges();
  int *verts, v1, v2;
  int i, j;
  int type;
  vtkIdType id, ptId;
  vtkIdType internalId[VTK_CELL_SIZE];
  double s1, s2, x[3], t, p1[3], p2[3], deltaScalar;
  int allInside=1, allOutside=1;
  
  // Create a triangulator if necessary.
  if ( ! this->Triangulator )
    {
    this->Triangulator = vtkOrderedTriangulator::New();
    this->Triangulator->PreSortedOff();
    this->Triangulator->UseTemplatesOn();
    this->ClipTetra = vtkTetra::New();
    this->ClipScalars = vtkDoubleArray::New();
    this->ClipScalars->SetNumberOfTuples(4);
    }

  // Make sure it's worth continuing by treating the interior and exterior
  // cases as special cases.
  for (i=0; i<numPts; i++)
    {
    s1 = cellScalars->GetComponent(i,0);
    if ( (s1 >= value && !insideOut) || (s1 < value && insideOut) )
      {
      allOutside = 0;
      }
    else
      {
      allInside = 0;
      }
    }

  if ( allOutside )
    {
    return;
    }

  // If here, the ordered triangulator is going to be used so the triangulation
  // has to be initialized.
  this->Triangulator->InitTriangulation(0.0,1.0, 0.0,1.0, 0.0,1.0,
                                        (numPts + numEdges));

  // Cells with fixed topology are triangulated with templates.
  double *p, *pPtr = this->GetParametricCoords();
  if ( this->IsPrimaryCell() )
    {
    // Some cell types support templates for interior clipping. Templates
    // are a heck of a lot faster.
    type = 0; //inside
    for (p=pPtr, i=0; i<numPts; i++, p+=3)
      {
      ptId = this->PointIds->GetId(i);
      this->Points->GetPoint(i, x);
      if ( locator->InsertUniquePoint(x, id) )
        {
        outPD->CopyData(inPD,ptId, id);
        }
      this->Triangulator->InsertPoint(id, x, p, type);
      }//for all cell points of fixed topology

    this->Triangulator->TemplateTriangulate(this->GetCellType(),
                                            numPts,numEdges);
    // If the cell is interior we are done.
    if ( allInside )
      {
      vtkIdType numTetras = tets->GetNumberOfCells();
      this->Triangulator->AddTetras(0,tets);
      vtkIdType numAddedTetras = tets->GetNumberOfCells() - numTetras;
      for (j=0; j<numAddedTetras; j++)
        {
        outCD->CopyData(inCD, cellId, numTetras+j);
        }
      }
    // Otherwise we have produced tetrahedra and now clip these using
    // the faster vtkTetra::Clip() method.
    else 
      {
      for ( this->Triangulator->InitTetraTraversal(); 
            this->Triangulator->GetNextTetra(0,this->ClipTetra,
                                             cellScalars,this->ClipScalars);)
        {
        // VERY IMPORTANT: Notice that the outPD is used twice. This is because the
        // tetra has been defined in terms of point ids that are defined in the
        // output (because of the templates).
        this->ClipTetra->Clip(value, this->ClipScalars, locator, tets, outPD,
                              outPD, inCD, cellId, outCD, insideOut);
        }
      }//if boundary cell
    return;
    } //if we are clipping fixed topology
  
  // If here we're left with a non-fixed topology cell (e.g. convex point set).
  // Inject cell points into triangulation. Recall that the PreSortedOff() 
  // flag was set which means that the triangulator will order the points 
  // according to point id.
  for (p=pPtr, i=0; i<numPts; i++, p+=3)
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
      type = 4; //outside, its type might change later (nearby intersection)
      }

    this->Points->GetPoint(i, x);
    if ( locator->InsertUniquePoint(x, id) )
      {
      outPD->CopyData(inPD,ptId, id);
      }
    internalId[i] = this->Triangulator->InsertPoint(id, x, p, type);
    }//for all points
  
  // For each edge intersection point, insert into triangulation. Edge
  // intersections come from clipping value. Have to be careful of 
  // intersections near exisiting points (causes bad Delaunay behavior).
  // Intersections near existing points are collapsed to existing point.
  double pc[3], *pc1, *pc2;
  for (int edgeNum=0; edgeNum < numEdges; edgeNum++)
    {
    cell3D->GetEdgePoints(edgeNum, verts);

    // Calculate a preferred interpolation direction.
    // Has to be done in same direction to insure coincident points are
    // merged (different interpolation direction causes pertubations).
    s1 = cellScalars->GetComponent(verts[0],0);
    s2 = cellScalars->GetComponent(verts[1],0);

    if ( (s1 <= value && s2 >= value) || (s1 >= value && s2 <= value) )
      {
      deltaScalar = s2 - s1;

      if (deltaScalar > 0)
        {
        v1 = verts[0]; v2 = verts[1];
        }
      else
        {
        v1 = verts[1]; v2 = verts[0];
        deltaScalar = -deltaScalar;
        }

      // linear interpolation
      t = ( deltaScalar == 0.0 ? 0.0 :
            (value - cellScalars->GetComponent(v1,0)) / deltaScalar );

      if ( t < this->MergeTolerance )
        {
        this->Triangulator->UpdatePointType(internalId[v1], 2);
        continue;
        }
      else if ( t > (1.0 - this->MergeTolerance) )
        {
        this->Triangulator->UpdatePointType(internalId[v2], 2);
        continue;
        }

      this->Points->GetPoint(v1, p1);
      this->Points->GetPoint(v2, p2);
      pc1 = pPtr + 3*v1;
      pc2 = pPtr + 3*v2;

      for (j=0; j<3; j++)
        {
        x[j] = p1[j] + t*(p2[j] - p1[j]);
        pc[j] = pc1[j] + t*(pc2[j] - pc1[j]);
        }
      
      // Incorporate point into output and interpolate edge data as necessary
      if ( locator->InsertUniquePoint(x, ptId) )
        {
        outPD->InterpolateEdge(inPD, ptId, this->PointIds->GetId(v1),
                               this->PointIds->GetId(v2), t);
        }

      //Insert intersection point into Delaunay triangulation
      this->Triangulator->InsertPoint(ptId,x,pc,2);

      }//if edge intersects value
    }//for all edges
  
  // triangulate the points
  this->Triangulator->Triangulate();

  // Add the triangulation to the mesh
  this->Triangulator->AddTetras(0,tets);
}

void vtkCell3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
}

// Note: the following code is placed here to deal with cross-library
// symbol export and import on Microsoft compilers.

//
// Edges to intersect.  Three at a time form a triangle. Comments at 
// end of line indicate case number (0->255) and base case number (0->15).
//
static vtkMarchingCubesTriangleCases  VTK_MARCHING_CUBES_TRICASES[] = { 
{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 0 0 */
{{ 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 1 1 */
{{ 0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 2 1 */
{{ 1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 3 2 */
{{ 1, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 4 1 */
{{ 0, 3, 8, 1, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 5 3 */
{{ 9, 11, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 6 2 */
{{ 2, 3, 8, 2, 8, 11, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 7 5 */
{{ 3, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 8 1 */
{{ 0, 2, 10, 8, 0, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 9 2 */
{{ 1, 0, 9, 2, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 10 3 */
{{ 1, 2, 10, 1, 10, 9, 9, 10, 8, -1, -1, -1, -1, -1, -1, -1}}, /* 11 5 */
{{ 3, 1, 11, 10, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 12 2 */
{{ 0, 1, 11, 0, 11, 8, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1}}, /* 13 5 */
{{ 3, 0, 9, 3, 9, 10, 10, 9, 11, -1, -1, -1, -1, -1, -1, -1}}, /* 14 5 */
{{ 9, 11, 8, 11, 10, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 15 8 */
{{ 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 16 1 */
{{ 4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 17 2 */
{{ 0, 9, 1, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 18 3 */
{{ 4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1}}, /* 19 5 */
{{ 1, 11, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 20 4 */
{{ 3, 7, 4, 3, 4, 0, 1, 11, 2, -1, -1, -1, -1, -1, -1, -1}}, /* 21 7 */
{{ 9, 11, 2, 9, 2, 0, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1}}, /* 22 7 */
{{ 2, 9, 11, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1}}, /* 23 14 */
{{ 8, 7, 4, 3, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 24 3 */
{{10, 7, 4, 10, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1}}, /* 25 5 */
{{ 9, 1, 0, 8, 7, 4, 2, 10, 3, -1, -1, -1, -1, -1, -1, -1}}, /* 26 6 */
{{ 4, 10, 7, 9, 10, 4, 9, 2, 10, 9, 1, 2, -1, -1, -1, -1}}, /* 27 9 */
{{ 3, 1, 11, 3, 11, 10, 7, 4, 8, -1, -1, -1, -1, -1, -1, -1}}, /* 28 7 */
{{ 1, 11, 10, 1, 10, 4, 1, 4, 0, 7, 4, 10, -1, -1, -1, -1}}, /* 29 11 */
{{ 4, 8, 7, 9, 10, 0, 9, 11, 10, 10, 3, 0, -1, -1, -1, -1}}, /* 30 12 */
{{ 4, 10, 7, 4, 9, 10, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1}}, /* 31 5 */
{{ 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 32 1 */
{{ 9, 4, 5, 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 33 3 */
{{ 0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 34 2 */
{{ 8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1}}, /* 35 5 */
{{ 1, 11, 2, 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 36 3 */
{{ 3, 8, 0, 1, 11, 2, 4, 5, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 37 6 */
{{ 5, 11, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1}}, /* 38 5 */
{{ 2, 5, 11, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1}}, /* 39 9 */
{{ 9, 4, 5, 2, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 40 4 */
{{ 0, 2, 10, 0, 10, 8, 4, 5, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 41 7 */
{{ 0, 4, 5, 0, 5, 1, 2, 10, 3, -1, -1, -1, -1, -1, -1, -1}}, /* 42 7 */
{{ 2, 5, 1, 2, 8, 5, 2, 10, 8, 4, 5, 8, -1, -1, -1, -1}}, /* 43 11 */
{{11, 10, 3, 11, 3, 1, 9, 4, 5, -1, -1, -1, -1, -1, -1, -1}}, /* 44 7 */
{{ 4, 5, 9, 0, 1, 8, 8, 1, 11, 8, 11, 10, -1, -1, -1, -1}}, /* 45 12 */
{{ 5, 0, 4, 5, 10, 0, 5, 11, 10, 10, 3, 0, -1, -1, -1, -1}}, /* 46 14 */
{{ 5, 8, 4, 5, 11, 8, 11, 10, 8, -1, -1, -1, -1, -1, -1, -1}}, /* 47 5 */
{{ 9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 48 2 */
{{ 9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1}}, /* 49 5 */
{{ 0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1}}, /* 50 5 */
{{ 1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 51 8 */
{{ 9, 8, 7, 9, 7, 5, 11, 2, 1, -1, -1, -1, -1, -1, -1, -1}}, /* 52 7 */
{{11, 2, 1, 9, 0, 5, 5, 0, 3, 5, 3, 7, -1, -1, -1, -1}}, /* 53 12 */
{{ 8, 2, 0, 8, 5, 2, 8, 7, 5, 11, 2, 5, -1, -1, -1, -1}}, /* 54 11 */
{{ 2, 5, 11, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1}}, /* 55 5 */
{{ 7, 5, 9, 7, 9, 8, 3, 2, 10, -1, -1, -1, -1, -1, -1, -1}}, /* 56 7 */
{{ 9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 10, 7, -1, -1, -1, -1}}, /* 57 14 */
{{ 2, 10, 3, 0, 8, 1, 1, 8, 7, 1, 7, 5, -1, -1, -1, -1}}, /* 58 12 */
{{10, 1, 2, 10, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1}}, /* 59 5 */
{{ 9, 8, 5, 8, 7, 5, 11, 3, 1, 11, 10, 3, -1, -1, -1, -1}}, /* 60 10 */
{{ 5, 0, 7, 5, 9, 0, 7, 0, 10, 1, 11, 0, 10, 0, 11, -1}}, /* 61 7 */
{{10, 0, 11, 10, 3, 0, 11, 0, 5, 8, 7, 0, 5, 0, 7, -1}}, /* 62 7 */
{{10, 5, 11, 7, 5, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 63 2 */
{{11, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 64 1 */
{{ 0, 3, 8, 5, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 65 4 */
{{ 9, 1, 0, 5, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 66 3 */
{{ 1, 3, 8, 1, 8, 9, 5, 6, 11, -1, -1, -1, -1, -1, -1, -1}}, /* 67 7 */
{{ 1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 68 2 */
{{ 1, 5, 6, 1, 6, 2, 3, 8, 0, -1, -1, -1, -1, -1, -1, -1}}, /* 69 7 */
{{ 9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1}}, /* 70 5 */
{{ 5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1}}, /* 71 11 */
{{ 2, 10, 3, 11, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 72 3 */
{{10, 8, 0, 10, 0, 2, 11, 5, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 73 7 */
{{ 0, 9, 1, 2, 10, 3, 5, 6, 11, -1, -1, -1, -1, -1, -1, -1}}, /* 74 6 */
{{ 5, 6, 11, 1, 2, 9, 9, 2, 10, 9, 10, 8, -1, -1, -1, -1}}, /* 75 12 */
{{ 6, 10, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1}}, /* 76 5 */
{{ 0, 10, 8, 0, 5, 10, 0, 1, 5, 5, 6, 10, -1, -1, -1, -1}}, /* 77 14 */
{{ 3, 6, 10, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1}}, /* 78 9 */
{{ 6, 9, 5, 6, 10, 9, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 79 5 */
{{ 5, 6, 11, 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 80 3 */
{{ 4, 0, 3, 4, 3, 7, 6, 11, 5, -1, -1, -1, -1, -1, -1, -1}}, /* 81 7 */
{{ 1, 0, 9, 5, 6, 11, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1}}, /* 82 6 */
{{11, 5, 6, 1, 7, 9, 1, 3, 7, 7, 4, 9, -1, -1, -1, -1}}, /* 83 12 */
{{ 6, 2, 1, 6, 1, 5, 4, 8, 7, -1, -1, -1, -1, -1, -1, -1}}, /* 84 7 */
{{ 1, 5, 2, 5, 6, 2, 3, 4, 0, 3, 7, 4, -1, -1, -1, -1}}, /* 85 10 */
{{ 8, 7, 4, 9, 5, 0, 0, 5, 6, 0, 6, 2, -1, -1, -1, -1}}, /* 86 12 */
{{ 7, 9, 3, 7, 4, 9, 3, 9, 2, 5, 6, 9, 2, 9, 6, -1}}, /* 87 7 */
{{ 3, 2, 10, 7, 4, 8, 11, 5, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 88 6 */
{{ 5, 6, 11, 4, 2, 7, 4, 0, 2, 2, 10, 7, -1, -1, -1, -1}}, /* 89 12 */
{{ 0, 9, 1, 4, 8, 7, 2, 10, 3, 5, 6, 11, -1, -1, -1, -1}}, /* 90 13 */
{{ 9, 1, 2, 9, 2, 10, 9, 10, 4, 7, 4, 10, 5, 6, 11, -1}}, /* 91 6 */
{{ 8, 7, 4, 3, 5, 10, 3, 1, 5, 5, 6, 10, -1, -1, -1, -1}}, /* 92 12 */
{{ 5, 10, 1, 5, 6, 10, 1, 10, 0, 7, 4, 10, 0, 10, 4, -1}}, /* 93 7 */
{{ 0, 9, 5, 0, 5, 6, 0, 6, 3, 10, 3, 6, 8, 7, 4, -1}}, /* 94 6 */
{{ 6, 9, 5, 6, 10, 9, 4, 9, 7, 7, 9, 10, -1, -1, -1, -1}}, /* 95 3 */
{{11, 9, 4, 6, 11, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 96 2 */
{{ 4, 6, 11, 4, 11, 9, 0, 3, 8, -1, -1, -1, -1, -1, -1, -1}}, /* 97 7 */
{{11, 1, 0, 11, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1}}, /* 98 5 */
{{ 8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 11, 1, -1, -1, -1, -1}}, /* 99 14 */
{{ 1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 100 5 */
{{ 3, 8, 0, 1, 9, 2, 2, 9, 4, 2, 4, 6, -1, -1, -1, -1}}, /* 101 12 */
{{ 0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 102 8 */
{{ 8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1}}, /* 103 5 */
{{11, 9, 4, 11, 4, 6, 10, 3, 2, -1, -1, -1, -1, -1, -1, -1}}, /* 104 7 */
{{ 0, 2, 8, 2, 10, 8, 4, 11, 9, 4, 6, 11, -1, -1, -1, -1}}, /* 105 10 */
{{ 3, 2, 10, 0, 6, 1, 0, 4, 6, 6, 11, 1, -1, -1, -1, -1}}, /* 106 12 */
{{ 6, 1, 4, 6, 11, 1, 4, 1, 8, 2, 10, 1, 8, 1, 10, -1}}, /* 107 7 */
{{ 9, 4, 6, 9, 6, 3, 9, 3, 1, 10, 3, 6, -1, -1, -1, -1}}, /* 108 11 */
{{ 8, 1, 10, 8, 0, 1, 10, 1, 6, 9, 4, 1, 6, 1, 4, -1}}, /* 109 7 */
{{ 3, 6, 10, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 110 5 */
{{ 6, 8, 4, 10, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 111 2 */
{{ 7, 6, 11, 7, 11, 8, 8, 11, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 112 5 */
{{ 0, 3, 7, 0, 7, 11, 0, 11, 9, 6, 11, 7, -1, -1, -1, -1}}, /* 113 11 */
{{11, 7, 6, 1, 7, 11, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1}}, /* 114 9 */
{{11, 7, 6, 11, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1}}, /* 115 5 */
{{ 1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1}}, /* 116 14 */
{{ 2, 9, 6, 2, 1, 9, 6, 9, 7, 0, 3, 9, 7, 9, 3, -1}}, /* 117 7 */
{{ 7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1}}, /* 118 5 */
{{ 7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 119 2 */
{{ 2, 10, 3, 11, 8, 6, 11, 9, 8, 8, 7, 6, -1, -1, -1, -1}}, /* 120 12 */
{{ 2, 7, 0, 2, 10, 7, 0, 7, 9, 6, 11, 7, 9, 7, 11, -1}}, /* 121 7 */
{{ 1, 0, 8, 1, 8, 7, 1, 7, 11, 6, 11, 7, 2, 10, 3, -1}}, /* 122 6 */
{{10, 1, 2, 10, 7, 1, 11, 1, 6, 6, 1, 7, -1, -1, -1, -1}}, /* 123 3 */
{{ 8, 6, 9, 8, 7, 6, 9, 6, 1, 10, 3, 6, 1, 6, 3, -1}}, /* 124 7 */
{{ 0, 1, 9, 10, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 125 4 */
{{ 7, 0, 8, 7, 6, 0, 3, 0, 10, 10, 0, 6, -1, -1, -1, -1}}, /* 126 3 */
{{ 7, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 127 1 */
{{ 7, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 128 1 */
{{ 3, 8, 0, 10, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 129 3 */
{{ 0, 9, 1, 10, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 130 4 */
{{ 8, 9, 1, 8, 1, 3, 10, 6, 7, -1, -1, -1, -1, -1, -1, -1}}, /* 131 7 */
{{11, 2, 1, 6, 7, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 132 3 */
{{ 1, 11, 2, 3, 8, 0, 6, 7, 10, -1, -1, -1, -1, -1, -1, -1}}, /* 133 6 */
{{ 2, 0, 9, 2, 9, 11, 6, 7, 10, -1, -1, -1, -1, -1, -1, -1}}, /* 134 7 */
{{ 6, 7, 10, 2, 3, 11, 11, 3, 8, 11, 8, 9, -1, -1, -1, -1}}, /* 135 12 */
{{ 7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 136 2 */
{{ 7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1}}, /* 137 5 */
{{ 2, 6, 7, 2, 7, 3, 0, 9, 1, -1, -1, -1, -1, -1, -1, -1}}, /* 138 7 */
{{ 1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1}}, /* 139 14 */
{{11, 6, 7, 11, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1}}, /* 140 5 */
{{11, 6, 7, 1, 11, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1}}, /* 141 9 */
{{ 0, 7, 3, 0, 11, 7, 0, 9, 11, 6, 7, 11, -1, -1, -1, -1}}, /* 142 11 */
{{ 7, 11, 6, 7, 8, 11, 8, 9, 11, -1, -1, -1, -1, -1, -1, -1}}, /* 143 5 */
{{ 6, 4, 8, 10, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 144 2 */
{{ 3, 10, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1}}, /* 145 5 */
{{ 8, 10, 6, 8, 6, 4, 9, 1, 0, -1, -1, -1, -1, -1, -1, -1}}, /* 146 7 */
{{ 9, 6, 4, 9, 3, 6, 9, 1, 3, 10, 6, 3, -1, -1, -1, -1}}, /* 147 11 */
{{ 6, 4, 8, 6, 8, 10, 2, 1, 11, -1, -1, -1, -1, -1, -1, -1}}, /* 148 7 */
{{ 1, 11, 2, 3, 10, 0, 0, 10, 6, 0, 6, 4, -1, -1, -1, -1}}, /* 149 12 */
{{ 4, 8, 10, 4, 10, 6, 0, 9, 2, 2, 9, 11, -1, -1, -1, -1}}, /* 150 10 */
{{11, 3, 9, 11, 2, 3, 9, 3, 4, 10, 6, 3, 4, 3, 6, -1}}, /* 151 7 */
{{ 8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 152 5 */
{{ 0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 153 8 */
{{ 1, 0, 9, 2, 4, 3, 2, 6, 4, 4, 8, 3, -1, -1, -1, -1}}, /* 154 12 */
{{ 1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1}}, /* 155 5 */
{{ 8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 11, -1, -1, -1, -1}}, /* 156 14 */
{{11, 0, 1, 11, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1}}, /* 157 5 */
{{ 4, 3, 6, 4, 8, 3, 6, 3, 11, 0, 9, 3, 11, 3, 9, -1}}, /* 158 7 */
{{11, 4, 9, 6, 4, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 159 2 */
{{ 4, 5, 9, 7, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 160 3 */
{{ 0, 3, 8, 4, 5, 9, 10, 6, 7, -1, -1, -1, -1, -1, -1, -1}}, /* 161 6 */
{{ 5, 1, 0, 5, 0, 4, 7, 10, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 162 7 */
{{10, 6, 7, 8, 4, 3, 3, 4, 5, 3, 5, 1, -1, -1, -1, -1}}, /* 163 12 */
{{ 9, 4, 5, 11, 2, 1, 7, 10, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 164 6 */
{{ 6, 7, 10, 1, 11, 2, 0, 3, 8, 4, 5, 9, -1, -1, -1, -1}}, /* 165 13 */
{{ 7, 10, 6, 5, 11, 4, 4, 11, 2, 4, 2, 0, -1, -1, -1, -1}}, /* 166 12 */
{{ 3, 8, 4, 3, 4, 5, 3, 5, 2, 11, 2, 5, 10, 6, 7, -1}}, /* 167 6 */
{{ 7, 3, 2, 7, 2, 6, 5, 9, 4, -1, -1, -1, -1, -1, -1, -1}}, /* 168 7 */
{{ 9, 4, 5, 0, 6, 8, 0, 2, 6, 6, 7, 8, -1, -1, -1, -1}}, /* 169 12 */
{{ 3, 2, 6, 3, 6, 7, 1, 0, 5, 5, 0, 4, -1, -1, -1, -1}}, /* 170 10 */
{{ 6, 8, 2, 6, 7, 8, 2, 8, 1, 4, 5, 8, 1, 8, 5, -1}}, /* 171 7 */
{{ 9, 4, 5, 11, 6, 1, 1, 6, 7, 1, 7, 3, -1, -1, -1, -1}}, /* 172 12 */
{{ 1, 11, 6, 1, 6, 7, 1, 7, 0, 8, 0, 7, 9, 4, 5, -1}}, /* 173 6 */
{{ 4, 11, 0, 4, 5, 11, 0, 11, 3, 6, 7, 11, 3, 11, 7, -1}}, /* 174 7 */
{{ 7, 11, 6, 7, 8, 11, 5, 11, 4, 4, 11, 8, -1, -1, -1, -1}}, /* 175 3 */
{{ 6, 5, 9, 6, 9, 10, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1}}, /* 176 5 */
{{ 3, 10, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1}}, /* 177 9 */
{{ 0, 8, 10, 0, 10, 5, 0, 5, 1, 5, 10, 6, -1, -1, -1, -1}}, /* 178 14 */
{{ 6, 3, 10, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1}}, /* 179 5 */
{{ 1, 11, 2, 9, 10, 5, 9, 8, 10, 10, 6, 5, -1, -1, -1, -1}}, /* 180 12 */
{{ 0, 3, 10, 0, 10, 6, 0, 6, 9, 5, 9, 6, 1, 11, 2, -1}}, /* 181 6 */
{{10, 5, 8, 10, 6, 5, 8, 5, 0, 11, 2, 5, 0, 5, 2, -1}}, /* 182 7 */
{{ 6, 3, 10, 6, 5, 3, 2, 3, 11, 11, 3, 5, -1, -1, -1, -1}}, /* 183 3 */
{{ 5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1}}, /* 184 11 */
{{ 9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1}}, /* 185 5 */
{{ 1, 8, 5, 1, 0, 8, 5, 8, 6, 3, 2, 8, 6, 8, 2, -1}}, /* 186 7 */
{{ 1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 187 2 */
{{ 1, 6, 3, 1, 11, 6, 3, 6, 8, 5, 9, 6, 8, 6, 9, -1}}, /* 188 7 */
{{11, 0, 1, 11, 6, 0, 9, 0, 5, 5, 0, 6, -1, -1, -1, -1}}, /* 189 3 */
{{ 0, 8, 3, 5, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 190 4 */
{{11, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 191 1 */
{{10, 11, 5, 7, 10, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 192 2 */
{{10, 11, 5, 10, 5, 7, 8, 0, 3, -1, -1, -1, -1, -1, -1, -1}}, /* 193 7 */
{{ 5, 7, 10, 5, 10, 11, 1, 0, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 194 7 */
{{11, 5, 7, 11, 7, 10, 9, 1, 8, 8, 1, 3, -1, -1, -1, -1}}, /* 195 10 */
{{10, 2, 1, 10, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1}}, /* 196 5 */
{{ 0, 3, 8, 1, 7, 2, 1, 5, 7, 7, 10, 2, -1, -1, -1, -1}}, /* 197 12 */
{{ 9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 10, -1, -1, -1, -1}}, /* 198 14 */
{{ 7, 2, 5, 7, 10, 2, 5, 2, 9, 3, 8, 2, 9, 2, 8, -1}}, /* 199 7 */
{{ 2, 11, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1}}, /* 200 5 */
{{ 8, 0, 2, 8, 2, 5, 8, 5, 7, 11, 5, 2, -1, -1, -1, -1}}, /* 201 11 */
{{ 9, 1, 0, 5, 3, 11, 5, 7, 3, 3, 2, 11, -1, -1, -1, -1}}, /* 202 12 */
{{ 9, 2, 8, 9, 1, 2, 8, 2, 7, 11, 5, 2, 7, 2, 5, -1}}, /* 203 7 */
{{ 1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 204 8 */
{{ 0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1}}, /* 205 5 */
{{ 9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1}}, /* 206 5 */
{{ 9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 207 2 */
{{ 5, 4, 8, 5, 8, 11, 11, 8, 10, -1, -1, -1, -1, -1, -1, -1}}, /* 208 5 */
{{ 5, 4, 0, 5, 0, 10, 5, 10, 11, 10, 0, 3, -1, -1, -1, -1}}, /* 209 14 */
{{ 0, 9, 1, 8, 11, 4, 8, 10, 11, 11, 5, 4, -1, -1, -1, -1}}, /* 210 12 */
{{11, 4, 10, 11, 5, 4, 10, 4, 3, 9, 1, 4, 3, 4, 1, -1}}, /* 211 7 */
{{ 2, 1, 5, 2, 5, 8, 2, 8, 10, 4, 8, 5, -1, -1, -1, -1}}, /* 212 11 */
{{ 0, 10, 4, 0, 3, 10, 4, 10, 5, 2, 1, 10, 5, 10, 1, -1}}, /* 213 7 */
{{ 0, 5, 2, 0, 9, 5, 2, 5, 10, 4, 8, 5, 10, 5, 8, -1}}, /* 214 7 */
{{ 9, 5, 4, 2, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 215 4 */
{{ 2, 11, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1}}, /* 216 9 */
{{ 5, 2, 11, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1}}, /* 217 5 */
{{ 3, 2, 11, 3, 11, 5, 3, 5, 8, 4, 8, 5, 0, 9, 1, -1}}, /* 218 6 */
{{ 5, 2, 11, 5, 4, 2, 1, 2, 9, 9, 2, 4, -1, -1, -1, -1}}, /* 219 3 */
{{ 8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1}}, /* 220 5 */
{{ 0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 221 2 */
{{ 8, 5, 4, 8, 3, 5, 9, 5, 0, 0, 5, 3, -1, -1, -1, -1}}, /* 222 3 */
{{ 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 223 1 */
{{ 4, 7, 10, 4, 10, 9, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1}}, /* 224 5 */
{{ 0, 3, 8, 4, 7, 9, 9, 7, 10, 9, 10, 11, -1, -1, -1, -1}}, /* 225 12 */
{{ 1, 10, 11, 1, 4, 10, 1, 0, 4, 7, 10, 4, -1, -1, -1, -1}}, /* 226 11 */
{{ 3, 4, 1, 3, 8, 4, 1, 4, 11, 7, 10, 4, 11, 4, 10, -1}}, /* 227 7 */
{{ 4, 7, 10, 9, 4, 10, 9, 10, 2, 9, 2, 1, -1, -1, -1, -1}}, /* 228 9 */
{{ 9, 4, 7, 9, 7, 10, 9, 10, 1, 2, 1, 10, 0, 3, 8, -1}}, /* 229 6 */
{{10, 4, 7, 10, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1}}, /* 230 5 */
{{10, 4, 7, 10, 2, 4, 8, 4, 3, 3, 4, 2, -1, -1, -1, -1}}, /* 231 3 */
{{ 2, 11, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1}}, /* 232 14 */
{{ 9, 7, 11, 9, 4, 7, 11, 7, 2, 8, 0, 7, 2, 7, 0, -1}}, /* 233 7 */
{{ 3, 11, 7, 3, 2, 11, 7, 11, 4, 1, 0, 11, 4, 11, 0, -1}}, /* 234 7 */
{{ 1, 2, 11, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 235 4 */
{{ 4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1}}, /* 236 5 */
{{ 4, 1, 9, 4, 7, 1, 0, 1, 8, 8, 1, 7, -1, -1, -1, -1}}, /* 237 3 */
{{ 4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 238 2 */
{{ 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 239 1 */
{{ 9, 8, 11, 11, 8, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 240 8 */
{{ 3, 9, 0, 3, 10, 9, 10, 11, 9, -1, -1, -1, -1, -1, -1, -1}}, /* 241 5 */
{{ 0, 11, 1, 0, 8, 11, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1}}, /* 242 5 */
{{ 3, 11, 1, 10, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 243 2 */
{{ 1, 10, 2, 1, 9, 10, 9, 8, 10, -1, -1, -1, -1, -1, -1, -1}}, /* 244 5 */
{{ 3, 9, 0, 3, 10, 9, 1, 9, 2, 2, 9, 10, -1, -1, -1, -1}}, /* 245 3 */
{{ 0, 10, 2, 8, 10, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 246 2 */
{{ 3, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 247 1 */
{{ 2, 8, 3, 2, 11, 8, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1}}, /* 248 5 */
{{ 9, 2, 11, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 249 2 */
{{ 2, 8, 3, 2, 11, 8, 0, 8, 1, 1, 8, 11, -1, -1, -1, -1}}, /* 250 3 */
{{ 1, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 251 1 */
{{ 1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 252 2 */
{{ 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 253 1 */
{{ 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, /* 254 1 */
{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}}; /* 255 0 */

vtkMarchingCubesTriangleCases* vtkMarchingCubesTriangleCases::GetCases()
{
  return VTK_MARCHING_CUBES_TRICASES;
}
