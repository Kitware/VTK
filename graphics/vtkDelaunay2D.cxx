/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelaunay2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkDelaunay2D.h"
#include "vtkMath.h"
#include "vtkTriangle.h"
#include "vtkPolygon.h"
#include "vtkPlane.h"

// Construct object with Alpha = 0.0; Tolerance = 0.001; Offset = 1.25;
// BoundingTriangulation turned off.
vtkDelaunay2D::vtkDelaunay2D()
{
  this->Alpha = 0.0;
  this->Tolerance = 0.001;
  this->BoundingTriangulation = 0;
  this->Offset = 1.0;
  this->Source = NULL;

  this->Output = vtkPolyData::New();
  this->Output->SetSource(this);
}

vtkDelaunay2D::~vtkDelaunay2D()
{
  this->SetSource(NULL);
}

// Override update method because execution can branch two ways (via Input 
// and Source).
void vtkDelaunay2D::Update()
{
  // make sure input is available
  if ( this->Input == NULL )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  if ( this->Source ) this->Source->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->ExecuteTime || 
  (this->Source && this->Source->GetMTime() > this->ExecuteTime) || 
  this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() ) this->Input->ForceUpdate();
    if ( this->Source && this->Source->GetDataReleased() ) 
      this->Source->ForceUpdate();

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Output->Initialize(); //clear output
    // reset AbortExecute flag and Progress
    this->AbortExecute = 0;
    this->Progress = 0.0;
    this->Execute();
    this->ExecuteTime.Modified();
    if ( !this->AbortExecute ) this->UpdateProgress(1.0);
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
  if ( this->Source && this->Source->ShouldIReleaseData() ) 
    this->Source->ReleaseData();
}

// Determine whether point x is inside of circumcircle of triangle
// defined by points (x1, x2, x3). Returns non-zero if inside circle.
// (Note that z-component is ignored.)
int vtkDelaunay2D::InCircle (float x[3], float x1[3], float x2[3], float x3[3])
{
  float radius2, center[2], dist2;

  radius2 = vtkTriangle::Circumcircle(x1,x2,x3,center);

  // check if inside/outside circumcircle
  dist2 = (x[0]-center[0]) * (x[0]-center[0]) + 
          (x[1]-center[1]) * (x[1]-center[1]);

  if ( dist2 < (0.99999*radius2) )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

#define VTK_DEL2D_TOLERANCE 1.0e-06

// Recursive method to locate triangle containing point. Starts with arbitrary
// triangle (tri) and "walks" towards it. Influenced by some of Guibas and 
// Stolfi's work. Returns id of enclosing triangle, or -1 if no triangle
// found. Also, the array nei[3] is used to communicate info about points
// that loe on triangle edges: nei[0] is neighboring triangle id, and nei[1]
// and nei[2] are the vertices defining the edge.
int vtkDelaunay2D::FindTriangle(float x[3], int ptIds[3], int tri, 
                                vtkPolyData *Mesh, vtkPoints *points, float tol, 
                                int nei[3])
{
  int i, j, npts, *pts, inside, i2, i3, newNei;
  vtkIdList *neighbors;
  float p[3][3], n[2], vp[2], vx[2], dp, minProj;
  
  // get local triangle info
  Mesh->GetCellPoints(tri,npts,pts);
  for (i=0; i<3; i++) 
    {
    ptIds[i] = pts[i];
    points->GetPoint(ptIds[i],p[i]);
    }

  // evaluate in/out of each edge
  for (inside=1, minProj=0.0, i=0; i<3; i++)
    {
    i2 = (i+1) % 3;
    i3 = (i+2) % 3;

    // create a 2D edge normal to define a "half-space"; evaluate points (i.e.,
    // candiate point and other triangle vertex not on this edge).
    n[0] = -(p[i2][1] - p[i][1]);
    n[1] = p[i2][0] - p[i][0];
    vtkMath::Normalize2D(n);

    // compute local vectors
    for (j=0; j<2; j++)
      {
      vp[j] = p[i3][j] - p[i][j];
      vx[j] = x[j] - p[i][j];
      }

    //check for duplicate point
    vtkMath::Normalize2D(vp);
    if ( vtkMath::Normalize2D(vx) <= tol ) 
      {
      this->NumberOfDuplicatePoints++;
      return -1;
      }

    // see if two points are in opposite half spaces
    dp = vtkMath::Dot2D(n,vp) * vtkMath::Dot2D(n,vx);
    if ( dp < VTK_DEL2D_TOLERANCE )
      {
      if ( dp < minProj ) //track edge most orthogonal to point direction
        {
        inside = 0;
        nei[1] = ptIds[i];
        nei[2] = ptIds[i2];
        minProj = dp;
        }
      }//outside this edge
    }//for each edge

  neighbors = vtkIdList::New(); neighbors->Allocate(2);
  if ( inside ) // all edges have tested positive
    {
    nei[0] = (-1);
    neighbors->Delete();
    return tri;
    }

  else if ( !inside && (fabs(minProj) < VTK_DEL2D_TOLERANCE) ) // on edge
    {
    Mesh->GetCellEdgeNeighbors(tri,nei[1],nei[2],neighbors);
    nei[0] = neighbors->GetId(0);
    neighbors->Delete();
    return tri;
    }

  else //walk towards point
    {
    Mesh->GetCellEdgeNeighbors(tri,nei[1],nei[2],neighbors);
    if ( (newNei=neighbors->GetId(0)) == nei[0] )
      {
      NumberOfDegeneracies++;
      neighbors->Delete();
      return -1;
      }
    else
      {
      nei[0] = tri;
      neighbors->Delete();
      return this->FindTriangle(x,ptIds,newNei,Mesh,points,tol,nei);
      }
    }
}

#undef VTK_DEL2D_TOLERANCE

// Recursive method checks whether edge is Delaunay, and if not, swaps edge.
// Continues until all edges are Delaunay. Points p1 and p2 form the edge in
// question; x is the coordinates of the inserted point; tri is the current
// triangle id; Mesh is a pointer to cell structure.
void vtkDelaunay2D::CheckEdge(int ptId, float x[3], int p1, int p2, int tri, 
                              vtkPolyData *Mesh, vtkPoints *points)
{
  int i, numNei, nei, npts, *pts, p3;
  float x1[3], x2[3], x3[3];
  vtkIdList *neighbors;
  int swapTri[3];

  points->GetPoint(p1,x1);
  points->GetPoint(p2,x2);

  neighbors = vtkIdList::New();
  neighbors->Allocate(2);

  Mesh->GetCellEdgeNeighbors(tri,p1,p2,neighbors);
  numNei = neighbors->GetNumberOfIds();

  if ( numNei > 0 ) //i.e., not a boundary edge
    {
    // get neighbor info including opposite point
    nei = neighbors->GetId(0);
    Mesh->GetCellPoints(nei, npts, pts);
    for (i=0; i<2; i++)
      {
      if ( pts[i] != p1 && pts[i] != p2 )
	{
        break;
	}
      }
    p3 = pts[i];
    points->GetPoint(p3,x3);

    // see whether point is in circumcircle
    if ( this->InCircle (x3, x, x1, x2) )
      {// swap diagonal
      Mesh->RemoveReferenceToCell(p1,tri);
      Mesh->RemoveReferenceToCell(p2,nei);
      Mesh->ResizeCellList(ptId,1);
      Mesh->AddReferenceToCell(ptId,nei);
      Mesh->ResizeCellList(p3,1);
      Mesh->AddReferenceToCell(p3,tri);

      swapTri[0] = ptId; swapTri[1] = p3; swapTri[2] = p2;
      Mesh->ReplaceCell(tri,3,swapTri);

      swapTri[0] = ptId; swapTri[1] = p1; swapTri[2] = p3;
      Mesh->ReplaceCell(nei,3,swapTri);

      // two new edges become suspect
      this->CheckEdge(ptId, x, p3, p2, tri, Mesh, points);
      this->CheckEdge(ptId, x, p1, p3, nei, Mesh, points);

      }//in circle
    }//interior edge

  neighbors->Delete();
}

// 2D Delaunay triangulation. Steps are as follows:
//   1. For each point
//   2. Find triangle point is in
//   3. Create 3 triangles from each edge of triangle that point is in
//   4. Recursively evaluate Delaunay criterion for each edge neighbor
//   5. If criterion not satisfied; swap diagonal
// 
void vtkDelaunay2D::Execute()
{
  int numPoints, numTriangles, i;
  int ptId, tri[4], nei[3], p1, p2;
  vtkPoints *inPoints;
  vtkPoints *points;
  vtkCellArray *triangles;
  vtkPolyData *Mesh;
  vtkPointSet *input=(vtkPointSet *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  float x[3];
  int nodes[4][3], pts[3], npts, *triPts, numNeiPts, *neiPts, ncells;
  vtkIdList *neighbors, *cells;
  float center[3], radius, tol;
  int *triUse = NULL;

  vtkDebugMacro(<<"Generating 2D Delaunay triangulation");

  // Initialize; check input
  //
  if ( (inPoints=input->GetPoints()) == NULL )
    {
    vtkErrorMacro("<<Cannot triangulate; no input points");
    return;
    }

  if ( (numPoints=inPoints->GetNumberOfPoints()) <= 2 )
    {
    vtkErrorMacro("<<Cannot triangulate; need at least 3 input points");
    return;
    }
  
  neighbors = vtkIdList::New(); neighbors->Allocate(2);
  cells = vtkIdList::New(); cells->Allocate(64);
  
  this->NumberOfDuplicatePoints = 0;
  this->NumberOfDegeneracies = 0;

  Mesh = vtkPolyData::New();
  
  // Create initial bounding triangulation. Have to create bounding points.
  // Initialize mesh structure.
  //
  points = vtkPoints::New(); points->SetNumberOfPoints(numPoints+8);
  for (ptId=0; ptId < numPoints; ptId++)
    {
    points->SetPoint(ptId,inPoints->GetPoint(ptId));
    }

  input->GetCenter(center);
  tol = input->GetLength();
  radius = this->Offset * tol;
  tol *= this->Tolerance;

  for (ptId=0; ptId<8; ptId++)
    {
    x[0] = center[0]
      + radius*cos((double)(45.0*ptId)*vtkMath::DegreesToRadians());
    x[1] = center[1]
      + radius*sin((double)(45.0*ptId)*vtkMath::DegreesToRadians());
    x[2] = center[2];
    points->SetPoint(numPoints+ptId,x);
    }

  triangles = vtkCellArray::New();
  triangles->Allocate(triangles->EstimateSize(2*numPoints,3));

  //create bounding triangles (there are six)
  pts[0] = numPoints; pts[1] = numPoints + 1; pts[2] = numPoints + 2;
  triangles->InsertNextCell(3,pts);
  pts[0] = numPoints + 2; pts[1] = numPoints + 3; pts[2] = numPoints + 4;
  triangles->InsertNextCell(3,pts);
  pts[0] = numPoints + 4; pts[1] = numPoints + 5; pts[2] = numPoints + 6;
  triangles->InsertNextCell(3,pts);
  pts[0] = numPoints + 6; pts[1] = numPoints + 7; pts[2] = numPoints + 0;
  triangles->InsertNextCell(3,pts);
  pts[0] = numPoints + 0; pts[1] = numPoints + 2; pts[2] = numPoints + 6;
  triangles->InsertNextCell(3,pts);
  pts[0] = numPoints + 2; pts[1] = numPoints + 4; pts[2] = numPoints + 6;
  triangles->InsertNextCell(3,pts);
  tri[0] = 0; //initialize value for FindTriangle

  Mesh->SetPoints(points);
  Mesh->SetPolys(triangles);
  Mesh->BuildLinks(); //build cell structure

  // For each point; find triangle containing point. Then evaluate three 
  // neighboring triangles for Delaunay criterion. Triangles that do not 
  // satisfy criterion have their edges swapped. This continues recursively 
  // until all triangles have been shown to be Delaunay.
  //
  for (ptId=0; ptId < numPoints; ptId++)
    {
    points->GetPoint(ptId,x); 
    nei[0] = (-1); //where we are coming from...nowhere initially

    if ( (tri[0] = this->FindTriangle(x,pts,tri[0],Mesh,points,tol,nei)) >= 0 )
      {
      if ( nei[0] < 0 ) //in triangle
        {
        //delete this triangle; create three new triangles
        //first triangle is replaced with one of the new ones
        nodes[0][0] = ptId; nodes[0][1] = pts[0]; nodes[0][2] = pts[1];
        Mesh->RemoveReferenceToCell(pts[2], tri[0]);
        Mesh->ReplaceCell(tri[0], 3, nodes[0]);
        Mesh->ResizeCellList(ptId,1);
        Mesh->AddReferenceToCell(ptId,tri[0]);

        //create two new triangles
        nodes[1][0] = ptId; nodes[1][1] = pts[1]; nodes[1][2] = pts[2];
        tri[1] = Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[1]);

        nodes[2][0] = ptId; nodes[2][1] = pts[2]; nodes[2][2] = pts[0];
        tri[2] = Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[2]);

        // Check edge neighbors for Delaunay criterion. If not satisfied, flip
        // edge diagonal. (This is done recursively.)
        this->CheckEdge(ptId, x, pts[0], pts[1], tri[0], Mesh, points);
        this->CheckEdge(ptId, x, pts[1], pts[2], tri[1], Mesh, points);
        this->CheckEdge(ptId, x, pts[2], pts[0], tri[2], Mesh, points);
        }

      else // on triangle edge
        {
        //update cell list
        Mesh->GetCellPoints(nei[0],numNeiPts,neiPts);
        for (i=0; i<3; i++)
          {
          if ( neiPts[i] != nei[1] && neiPts[i] != nei[2] ) 
            {
            p1 = neiPts[i];
            }
          if ( pts[i] != nei[1] && pts[i] != nei[2] ) 
            {
            p2 = pts[i];
            }
          }
        Mesh->ResizeCellList(p1,1);
        Mesh->ResizeCellList(p2,1);

        //replace two triangles
        Mesh->RemoveReferenceToCell(nei[2],tri[0]);
        Mesh->RemoveReferenceToCell(nei[2],nei[0]);
        nodes[0][0] = ptId; nodes[0][1] = p1; nodes[0][2] = nei[1];
        Mesh->ReplaceCell(tri[0], 3, nodes[0]);
        nodes[1][0] = ptId; nodes[1][1] = p2; nodes[1][2] = nei[1];
        Mesh->ReplaceCell(nei[0], 3, nodes[1]);
        Mesh->ResizeCellList(ptId, 2);
        Mesh->AddReferenceToCell(ptId,tri[0]);
        Mesh->AddReferenceToCell(ptId,nei[0]);

        //create two new triangles
        nodes[2][0] = ptId; nodes[2][1] = p2; nodes[2][2] = nei[2];
        tri[2] = Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[2]);

        nodes[3][0] = ptId; nodes[3][1] = p1; nodes[3][2] = nei[2];
        tri[3] = Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[3]);

        // Check edge neighbors for Delaunay criterion.
        for ( i=0; i<4; i++ )
          {
          this->CheckEdge (ptId, x, nodes[i][1], nodes[i][2], tri[i], 
                           Mesh, points);
          }
        }
      }//if triangle found

    else
      {
      tri[0] = 0; //no triangle found
      }

    if ( ! (ptId % 1000) ) 
      {
      vtkDebugMacro(<<"point #" << ptId);
      this->UpdateProgress ((float)ptId/numPoints);
      if (this->GetAbortExecute()) 
        {
        break;
        }
      }
    
    }//for all points

  vtkDebugMacro(<<"Triangulated " << numPoints <<" points, " 
                << this->NumberOfDuplicatePoints << " of which were duplicates");

  if ( this->NumberOfDegeneracies > 0 )
    {
    vtkWarningMacro(<< this->NumberOfDegeneracies 
                 << " degenerate triangles encountered, mesh quality suspect");
    }

  // Finish up by recovering the boundary, or deleting all triangles connected 
  // to the bounding triangulation points or not satisfying alpha criterion,
  if ( !this->BoundingTriangulation || this->Alpha > 0.0 || this->Source )
    {
    numTriangles = Mesh->GetNumberOfCells();
    if ( this->Source ) 
      {
      triUse = this->RecoverBoundary(Mesh);
      }
    else
      {
      triUse = new int[numTriangles];
      for (i=0; i<numTriangles; i++) 
        {
        triUse[i] = 1;
        }
      }
    }

  // Delete triangles connected to boundary points (if not desired)
  if ( ! this->BoundingTriangulation )
    {
    for (ptId=numPoints; ptId < (numPoints+8); ptId++)
      {
      Mesh->GetPointCells(ptId, cells);
      ncells = cells->GetNumberOfIds();
      for (i=0; i < ncells; i++)
        {
        triUse[cells->GetId(i)] = 0; //mark as deleted
        }
      }
    }

  // If non-zero alpha value, then figure out which parts of mesh are
  // contained within alpha radius.
  //
  if ( this->Alpha > 0.0 )
    {
    float alpha2 = this->Alpha * this->Alpha;
    float x1[3], x2[3], x3[3];
    int j, cellId, numNei, p1, p2, nei;

    vtkCellArray *alphaVerts = vtkCellArray::New();
    alphaVerts->Allocate(numPoints);
    vtkCellArray *alphaLines = vtkCellArray::New();
    alphaLines->Allocate(numPoints);

    char *pointUse = new char[numPoints+8];
    for (ptId=0; ptId < (numPoints+8); ptId++) pointUse[ptId] = 0;

    //traverse all triangles; evaluating Delaunay criterion
    for (i=0; i < numTriangles; i++)
      {
      if ( triUse[i] == 1 )
        {
        Mesh->GetCellPoints(i, npts, triPts);
        points->GetPoint(triPts[0],x1);
        points->GetPoint(triPts[1],x2);
        points->GetPoint(triPts[2],x3);
        if ( vtkTriangle::Circumcircle(x1,x2,x3,center) > alpha2 )
          {
          triUse[i] = 0;
          }
        else
          {
          for (j=0; j<3; j++)
	    {
	    pointUse[triPts[j]] = 1; 
	    }
          }
        }//if non-deleted triangle
      }//for all triangles

    //traverse all edges see whether we need to create some
    for (cellId=0, triangles->InitTraversal(); 
    triangles->GetNextCell(npts,triPts); cellId++)
      {
      if ( ! triUse[cellId] )
        {
        for (i=0; i < npts; i++) 
          {
          p1 = triPts[i];
          p2 = triPts[(i+1)%npts];

          if (this->BoundingTriangulation || (p1<numPoints && p2<numPoints))
            {
            Mesh->GetCellEdgeNeighbors(cellId,p1,p2,neighbors);
            numNei = neighbors->GetNumberOfIds();

            if ( numNei < 1 || ((nei=neighbors->GetId(0)) > cellId 
                                && !triUse[nei]) )
              {//see whether edge is shorter than Alpha
              points->GetPoint(p1,x1);
              points->GetPoint(p2,x2);
              if ( (vtkMath::Distance2BetweenPoints(x1,x2)*0.25) <= alpha2 )
                {
                pointUse[p1] = 1; pointUse[p2] = 1;
                pts[0] = p1;
                pts[1] = p2;
                alphaLines->InsertNextCell(2,pts);
                }//if passed test
              }//test edge
            }//if valid edge
          }//for all edges of this triangle
        }//if triangle not output
      }//for all triangles

    //traverse all points, create vertices if none used
    for (ptId=0; ptId<(numPoints+8); ptId++)
      {
      if ( !pointUse[ptId]
           &&  (ptId < numPoints || this->BoundingTriangulation) )
        {
        pts[0] = ptId;
        alphaVerts->InsertNextCell(1,pts);
        }
      }

    // update output
    delete [] pointUse;
    output->SetVerts(alphaVerts);
    alphaVerts->Delete();
    output->SetLines(alphaLines);
    alphaLines->Delete();
    }

  // Update output; free up supporting data structures.
  //
  if ( this->BoundingTriangulation )
    {
    output->SetPoints(points);
    }  
  else
    {
    output->SetPoints(inPoints);
    output->GetPointData()->PassData(input->GetPointData());
    }

  if ( this->Alpha <= 0.0 && this->BoundingTriangulation && !this->Source )
    {
    output->SetPolys(triangles);
    }
  else
    {
    vtkCellArray *alphaTriangles = vtkCellArray::New();
    alphaTriangles->Allocate(numTriangles);
    int *alphaTriPts;

    for (i=0; i<numTriangles; i++)
      {
      if ( triUse[i] )
        {
        Mesh->GetCellPoints(i,npts,alphaTriPts);
        alphaTriangles->InsertNextCell(3,alphaTriPts);
        }
      }
    output->SetPolys(alphaTriangles);
    alphaTriangles->Delete();
    delete [] triUse;
    }

  points->Delete();
  triangles->Delete();
  Mesh->Delete();
  neighbors->Delete();
  cells->Delete();

  output->Squeeze();
}

// Methods used to recover edges. Uses lines and polygons to determine boundary
// and inside/outside.
int *vtkDelaunay2D::RecoverBoundary(vtkPolyData *Mesh)
{
  vtkPolyData *source=this->GetSource();
  vtkCellArray *lines=source->GetLines();
  vtkCellArray *polys=source->GetPolys();
  int i, npts, *pts, p1, p2;
  int *triUse;
  
  // Recover the edges of the mesh
  for ( lines->InitTraversal(); lines->GetNextCell(npts,pts); )
    {
    for (i=0; i<(npts-1); i++)
      {
      p1 = pts[i];
      p2 = pts[i+1];
      if ( ! Mesh->IsEdge(p1,p2) )
        {
        this->RecoverEdge(Mesh, p1, p2);
        }
      }
    }

  // Recover the enclosed regions (polygons) of the mesh
  for ( polys->InitTraversal(); polys->GetNextCell(npts,pts); )
    {
    for (i=0; i<npts; i++)
      {
      p1 = pts[i];
      p2 = pts[(i+1)%npts];
      if ( ! Mesh->IsEdge(p1,p2) )
        {
        this->RecoverEdge(Mesh, p1, p2);
        }
      }
    }
  
  // Generate inside/outside marks on mesh
  int numTriangles = Mesh->GetNumberOfCells();
  triUse = new int[numTriangles];
  for (i=0; i<numTriangles; i++) 
    {
    triUse[i] = 1;
    }
  
  // Use any polygons to mark inside and outside. (Note that if an edge was not
  // recovered, we're going to have a problem.) The first polygon is assumed to
  // define the outside of the polygon; additional polygons carve out inside
  // holes.
  this->FillPolygons(Mesh, polys, triUse);

  return triUse;
}

// Method attempts to recover an edge by retriangulating mesh around the edge.
// What we do is identify a "submesh" of triangles that includes the edge to recover.
// Then we split the submesh in two with the recovered edge, and triangulate each of
// the two halves. If any part of this fails, we leave things alone.
int vtkDelaunay2D::RecoverEdge(vtkPolyData *Mesh, int p1, int p2)
{
  int i, j, k, cellId;
  float p1X[3], p2X[3], xyNormal[3], splitNormal[3], p21[3];
  float x1[3], x2[3], sepNormal[3], v21[3];
  int npts, *pts, ncells, v1, v2, signX1, signX2, signP1, signP2;
  int success=0, numRightTris, numLeftTris, *rightTris, *leftTris;

  vtkIdList *cells=vtkIdList::New(); cells->Allocate(64);  
  vtkIdList *tris=vtkIdList::New(); tris->Allocate(64);
  vtkPolygon *rightPoly=vtkPolygon::New();
  vtkPolygon *leftPoly=vtkPolygon::New();
  vtkIdList *leftChain=leftPoly->GetPointIds();
  vtkIdList *rightChain=rightPoly->GetPointIds();
  vtkPoints *leftChainX=leftPoly->GetPoints();
  vtkPoints *rightChainX=rightPoly->GetPoints();
  vtkIdList *neis=vtkIdList::New(); neis->Allocate(4);
  vtkIdList *rightPtIds=vtkIdList::New(); rightPtIds->Allocate(64);
  vtkIdList *leftPtIds=vtkIdList::New(); leftPtIds->Allocate(64);
  vtkPoints *rightTriPts=vtkPoints::New(); rightTriPts->Allocate(64);
  vtkPoints *leftTriPts=vtkPoints::New(); leftTriPts->Allocate(64);
  
  // Compute a split plane along (p1,p2) and parallel to the z-axis.
  //
  Mesh->GetPoint(p1,p1X); p1X[2] = 0.0; //split plane point
  Mesh->GetPoint(p2,p2X); p2X[2] = 0.0; //split plane point
  xyNormal[0] = xyNormal[1] = 0.0; xyNormal[2] = 1.0;
  for (i=0; i<3; i++ ) 
    {
    p21[i] = p2X[i] - p1X[i]; //working in x-y plane
    }

  vtkMath::Cross (p21,xyNormal,splitNormal);
  if ( vtkMath::Normalize(splitNormal) == 0.0 ) 
    {//Usually means coincident points
    goto FAILURE;
    }

  // Identify a triangle connected to the point p1 containing a portion of the edge.
  //
  Mesh->GetPointCells(p1, cells);
  ncells = cells->GetNumberOfIds();
  for (i=0; i < ncells; i++)
    {
    cellId = cells->GetId(i);
    Mesh->GetCellPoints(cellId, npts, pts);
    for (j=0; j<3; j++)
      {
      if ( pts[j] == p1 ) 
        {
        break;
        }
      }
    v1 = pts[(j+1)%3];
    v2 = pts[(j+2)%3];
    Mesh->GetPoint(v1,x1); x1[2] = 0.0;
    Mesh->GetPoint(v2,x2); x2[2] = 0.0;
    signX1 = (vtkPlane::Evaluate(splitNormal, p1X, x1) > 0.0 ? 1 : -1);
    signX2 = (vtkPlane::Evaluate(splitNormal, p1X, x2) > 0.0 ? 1 : -1);
    if ( signX1 != signX2 ) //points of triangle on either side of edge
      {
      // determine if edge separates p1 from p2 - then we've found triangle
      v21[0] = x2[0] - x1[0]; //working in x-y plane
      v21[1] = x2[1] - x1[1];
      v21[2] = 0.0;

      vtkMath::Cross (v21,xyNormal,sepNormal);
      if ( vtkMath::Normalize(sepNormal) == 0.0 )
        { //bad mesh
        goto FAILURE;
        }

      signP1 = (vtkPlane::Evaluate(sepNormal, x1, p1X) > 0.0 ? 1 : -1);
      signP2 = (vtkPlane::Evaluate(sepNormal, x1, p2X) > 0.0 ? 1 : -1);
      if ( signP1 != signP2 ) //is a separation line
        {
        break;
        }
      }
    } //for all cells

  if ( i >= ncells ) 
    {//something is really screwed up
    goto FAILURE;
    }

  // We found initial triangle; begin to track triangles containing edge. Also,
  // the triangle defines the beginning of two "chains" which form a boundary of
  // enclosing triangles around the edge. Create the two chains (from p1 to p2).
  // (The chains are actually defining two polygons on either side of the edge.)
  // 
  tris->InsertId(0, cellId);
  rightChain->InsertId(0, p1); rightChainX->InsertPoint(0, p1X);
  leftChain->InsertId(0, p1); leftChainX->InsertPoint(0, p1X);
  if ( signX1 > 0 )
    {
    rightChain->InsertId(1, v1); rightChainX->InsertPoint(1, x1);
    leftChain->InsertId(1, v2); leftChainX->InsertPoint(1, x2);
    }
  else
    {
    leftChain->InsertId(1, v1); leftChainX->InsertPoint(1, x1);
    rightChain->InsertId(1, v2); rightChainX->InsertPoint(1, x2);
    }
  
  // Walk along triangles (edge neighbors) towards point p2.
  while ( v1 != p2 )
    {
    Mesh->GetCellEdgeNeighbors(cellId, v1, v2, neis);
    if ( neis->GetNumberOfIds() != 1 )
      {//Mesh is folded or degenerate
      goto FAILURE;
      }
    cellId = neis->GetId(0);
    tris->InsertNextId(cellId);
    Mesh->GetCellPoints(cellId, npts, pts);
    for (j=0; j<3; j++)
      {
      if ( pts[j] != v1 && pts[j] != v2 )
        {//found point opposite current edge (v1,v2)
        if ( pts[j] == p2 )
          {
          v1 = p2; //this will cause the walk to stop
          rightChain->InsertNextId(p2); rightChainX->InsertNextPoint(p2X);
          leftChain->InsertNextId(p2); leftChainX->InsertNextPoint(p2X);
          }
        else
          {//keep walking
          Mesh->GetPoint(pts[j], x1); x1[2] = 0.0;
          if ( vtkPlane::Evaluate(splitNormal, p1X, x1) > 0.0 )
            {
            v1 = pts[j];
            rightChain->InsertNextId(v1); rightChainX->InsertNextPoint(x1);
            }
          else
            {
            v2 = pts[j];
            leftChain->InsertNextId(v2); leftChainX->InsertNextPoint(x1);
            }
          }
        break;
        }//else found opposite point
      }//for all points in triangle
    }//while walking
  
  // Now that the to chains are formed, each chain forms a polygon (along with
  // the edge (p1,p2)) that requires triangulation. If we can successfully
  // triangulate the two polygons, we will delete the triangles contained within
  // the chains and replace them with the new triangulation.
  //
  success = 1;
  success &= (rightPoly->Triangulate(0, rightPtIds, rightTriPts));
  numRightTris = rightPtIds->GetNumberOfIds() / 3;

  success &= (leftPoly->Triangulate(0, leftPtIds, leftTriPts));
  numLeftTris = leftPtIds->GetNumberOfIds() / 3;
  
  if ( ! success ) 
    {//polygons on either side of edge are poorly shaped
    goto FAILURE;
    }
  
  // Okay, delete the old triangles and replace them with new ones. There should be
  // the same number of new triangles as old ones.
  leftTris = leftPtIds->GetPointer(0);
  for ( j=i=0; i<numLeftTris; i++, j++, leftTris+=3)
    {
    cellId = tris->GetId(j);
    Mesh->RemoveCellReference(cellId);
    for (k=0; k<3; k++)
      {//allocate new space for cell lists
      Mesh->ResizeCellList(leftTris[k],1);
      }
    Mesh->ReplaceLinkedCell(cellId, 3, leftTris);
    }

  rightTris = rightPtIds->GetPointer(0);
  for ( i=0; i<numRightTris; i++, j++, rightTris+=3)
    {
    cellId = tris->GetId(j);
    Mesh->RemoveCellReference(cellId);
    for (k=0; k<3; k++)
      {//allocate new space for cell lists
      Mesh->ResizeCellList(rightTris[k],1);
      }
    Mesh->ReplaceLinkedCell(cellId, 3, rightTris);
    }

  FAILURE:
    tris->Delete(); cells->Delete();
    leftPoly->Delete(); rightPoly->Delete(); neis->Delete();
    rightPtIds->Delete(); leftPtIds->Delete();
    rightTriPts->Delete(); leftTriPts->Delete();
    return success;
}

void vtkDelaunay2D::FillPolygons(vtkPolyData *Mesh, vtkCellArray *polys, int *triUse)
{
  int npts, *pts, p1, p2, i, j, k, kk;
  static float xyNormal[3]={0.0,0.0,1.0};
  float negDir[3], x21[3], x1[3], x2[3], x[3];
  vtkIdList *neis=vtkIdList::New();
  int cellId, numNeis, numPts, *triPts;
  vtkIdList *currentFront = vtkIdList::New(), *tmpFront;
  vtkIdList *nextFront = vtkIdList::New();
  int numCellsInFront, neiId, numTriangles=Mesh->GetNumberOfCells();

  // Loop over edges of polygon, marking triangles on "outside" of polygon as outside.
  // Then perform a fill.
  for ( polys->InitTraversal(); polys->GetNextCell(npts,pts); )
    {
    currentFront->Reset();
    for (i=0; i<npts; i++)
      {
      p1 = pts[i];
      p2 = pts[(i+1)%npts];
      if ( ! Mesh->IsEdge(p1,p2) )
        {
        vtkWarningMacro(<<"Edge not recovered, polygon fill suspect");
        }
      else //Mark the "outside" triangles
        {
        neis->Reset();
        Mesh->GetPoint(p1,x1);
        Mesh->GetPoint(p2,x2);
        for (j=0; j<3; j++)
          {
          x21[j] = x2[j] - x1[j];
          }
        vtkMath::Cross (x21,xyNormal,negDir);
        Mesh->GetCellEdgeNeighbors(-1, p1, p2, neis); //get both triangles
        numNeis = neis->GetNumberOfIds();
        for (j=0; j<numNeis; j++)
          {//find the vertex not on the edge; evaluate it (and the cell) in/out
          cellId = neis->GetId(j);
          Mesh->GetCellPoints(cellId, numPts, triPts);
          for (k=0; k<3; k++)
            {
            if ( triPts[k] != p1 && triPts[k] != p2 ) 
              {
              break;
              }
            }
          Mesh->GetPoint(triPts[k],x); x[2] = 0.0;
          if ( vtkPlane::Evaluate(negDir, x1, x) > 0.0 )
            {
            triUse[cellId] = 0;
            currentFront->InsertNextId(cellId);
            }
          else
            {
            triUse[cellId] = -1;
            }
          }
        }//edge was recovered
      }//for all edges in polygon

    // Okay, now perform a fill operation (filling "outside" values).
    //
    while ( (numCellsInFront = currentFront->GetNumberOfIds()) > 0 )
      {
      for (j=0; j < numCellsInFront; j++)
        {
        cellId = currentFront->GetId(j);

        Mesh->GetCellPoints(cellId, numPts, triPts);
        for (k=0; k<3; k++)
          {
          p1 = triPts[k];
          p2 = triPts[(k+1)%3];

          Mesh->GetCellEdgeNeighbors(cellId, p1, p2, neis);
          numNeis = neis->GetNumberOfIds();
          for (kk=0; kk<numNeis; kk++)
            {
            neiId = neis->GetId(kk);
            if ( triUse[neiId] == 1 ) //0 is what we're filling with
              {
              triUse[neiId] = 0;
              nextFront->InsertNextId(neiId);
              }
            }//mark all neigbors
          }//for all edges of cell
        } //all cells in front

      tmpFront = currentFront;
      currentFront = nextFront;
      nextFront = tmpFront;
      nextFront->Reset();
      } //while still advancing

    }//for all polygons

  //convert all unvisited to inside
  for (i=0; i<numTriangles; i++) 
    {
    if ( triUse[i] == -1 )
      {
      triUse[i] = 1;
      }
    }

  currentFront->Delete();
  nextFront->Delete();
  neis->Delete();
}

void vtkDelaunay2D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetFilter::PrintSelf(os,indent);

  os << indent << "Source: " << (void *)this->Source << "\n";
  os << indent << "Alpha: " << this->Alpha << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Bounding Triangulation: "
     << (this->BoundingTriangulation ? "On\n" : "Off\n");
}
