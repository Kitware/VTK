/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelaunay2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkDelaunay2D.hh"
#include "vtkMath.hh"
#include "vtkTriangle.hh"

// Description:
// Construct object with Alpha = 0.0; Tolerance = 0.001; Offset = 1.25;
// BoundingTriangulation turned off.
vtkDelaunay2D::vtkDelaunay2D()
{
  this->Alpha = 0.0;
  this->Tolerance = 0.001;
  this->BoundingTriangulation = 0;
  this->Offset = 1.0;

  this->Output = new vtkPolyData;
  this->Output->SetSource(this);
}

// Determine whether point x is inside of circumcircle of triangle
// defined by points (x1, x2, x3). Returns non-zero if inside circle.
static int InCircle (float x[3], float x1[3], float x2[3], float x3[3])
{
  float radius2, center[2], dist2;

  radius2 = vtkTriangle::Circumcircle(x1,x2,x3,center);

  // check if inside/outside circumcircle
  dist2 = (x[0]-center[0]) * (x[0]-center[0]) + 
          (x[1]-center[1]) * (x[1]-center[1]);

  if ( dist2 < (0.9999*radius2) ) return 1;
  else return 0;
}

static int NumberOfDuplicatePoints;

// Recursive method to locate triangle containing point. Starts with arbitrary
// triangle (tri) and "walks" towards it. Influenced by some of Guibas and 
// Stolfi's work. Returns id of enclosing triangle, or -1 if no triangle found.
static int FindTriangle(float x[3], int ptIds[3], int tri, vtkPolyData *Mesh, 
                        vtkFloatPoints *points, float tol)
{
  int i, j, npts, *pts, inside, i2, i3, nei[2];
  vtkIdList neighbors(2);
  float p[3][3], v12[3], vp[3], vx[3], v1[3], v2[3], dp, minProj;
  
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

    // compute normal and local vectors
    for (j=0; j<3; j++)
      {
      v12[j] = p[i2][j] - p[i][j];
      vp[j] = p[i3][j] - p[i][j];
      vx[j] = x[j] - p[i][j];
      }

    if ( vtkMath::Normalize(vx) <= tol ) //check for duplicate point
      {
      NumberOfDuplicatePoints++;
      return -1;
      }

    // create two vectors: normal to edge and vector to point
    vtkMath::Cross(vp,v12,v1); vtkMath::Normalize(v1);
    vtkMath::Cross(vx,v12,v2); vtkMath::Normalize(v2);

    // see if point is on opposite side of edge
    if ( (dp=vtkMath::Dot(v1,v2)) < -1.0e-04 )
      {
      if ( dp < minProj ) //track edge most orthogonal to point direction
        {
        inside = 0;
        nei[0] = ptIds[i];
        nei[1] = ptIds[i2];
        }
      }//outside this edge
    }//for each edge

  //if not inside, walk towards point
  if ( !inside )
    {
    Mesh->GetCellEdgeNeighbors(tri,nei[0],nei[1],neighbors);
    return FindTriangle(x,ptIds,neighbors.GetId(0),Mesh,points,tol);
    }
  else //must be in this triangle if all edges test inside
    {
    return tri;
    }
}

// Recursive method checks whether edge is Delaunay, and if not, swaps edge.
// Continues until all edges are Delaunay. Points p1 and p2 form the edge in
// question; x is the coordinates of the inserted point; tri is the current
// triangle id; Mesh is a pointer to cell structure.
static void CheckEdge(int ptId, float x[3], int p1, int p2, int tri, 
              vtkPolyData *Mesh, vtkFloatPoints *points)
{
  int i, numNei, nei, npts, *pts, p3;
  float x1[3], x2[3], x3[3];
  vtkIdList neighbors(2);
  int swapTri[3];

  points->GetPoint(p1,x1);
  points->GetPoint(p2,x2);

  Mesh->GetCellEdgeNeighbors(tri,p1,p2,neighbors);
  numNei = neighbors.GetNumberOfIds();

  if ( numNei > 0 ) //i.e., not a boundary edge
    {
    // get neighbor info including opposite point
    nei = neighbors.GetId(0);
    Mesh->GetCellPoints(nei, npts, pts);
    for (i=0; i<2; i++)
      if ( pts[i] != p1 && pts[i] != p2 )
        break;

    p3 = pts[i];
    points->GetPoint(p3,x3);

    // see whether point is in circumcircle
    if ( InCircle (x3, x, x1, x2) )
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
      CheckEdge(ptId, x, p2, p3, tri, Mesh, points);
      CheckEdge(ptId, x, p3, p1, nei, Mesh, points);

      }//in circle
    }//interior edge
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
  int ptId, tri[3];
  vtkPoints *inPoints;
  vtkFloatPoints *points;
  vtkCellArray *triangles;
  vtkPolyData *Mesh=new vtkPolyData;
  vtkPointSet *input=(vtkPointSet *)this->Input;
  vtkPolyData *output=(vtkPolyData *)this->Output;
  float x[3];
  int nodes[3][3], pts[3], npts, *triPts;
  vtkIdList neighbors(2), cells(64);
  float center[3], radius, tol;
  char *triUse = NULL;

  vtkDebugMacro(<<"Generating 2D Delaunay triangulation");
//
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
  NumberOfDuplicatePoints = 0;
//
// Create initial bounding triangulation. Have to create bounding points.
// Initialize mesh structure.
//
  points = new vtkFloatPoints(numPoints+8);
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
    x[0] = center[0] + radius*cos((double)(45.0*ptId)*vtkMath::DegreesToRadians());
    x[1] = center[1] + radius*sin((double)(45.0*ptId)*vtkMath::DegreesToRadians());
    x[2] = 0.0;
    points->SetPoint(numPoints+ptId,x);
    }

  triangles = new vtkCellArray;
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
//
// For each point; find triangle containing point. Then evaluate three 
// neighboring triangles for Delaunay criterion. Triangles that do not 
// satisfy criterion have their edges swapped. This continues recursively 
// until all triangles have been shown to be Delaunay.
//
  for (ptId=0; ptId < numPoints; ptId++)
    {
    points->GetPoint(ptId,x);
    if ( (tri[0] = FindTriangle(x,pts,tri[0],Mesh,points,tol)) >= 0 )
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
      CheckEdge(ptId, x, pts[0], pts[1], tri[0], Mesh, points);
      CheckEdge(ptId, x, pts[1], pts[2], tri[1], Mesh, points);
      CheckEdge(ptId, x, pts[2], pts[0], tri[2], Mesh, points);

      }//if triangle found
    else
      {
      tri[0] = 0; //reset starting location
      }
    }//for all points

  vtkDebugMacro(<<"Triangulated " << numPoints <<" points, " 
                << NumberOfDuplicatePoints << " of which were duplicates");
//
// Finish up by deleting all triangles connected to initial triangulation
//
  numTriangles = Mesh->GetNumberOfCells();
  if ( !this->BoundingTriangulation || this->Alpha > 0.0 )
    {
    triUse = new char[numTriangles];
    for (i=0; i<numTriangles; i++) triUse[i] = 1;
    }

  if ( ! this->BoundingTriangulation )
    {
    for (ptId=numPoints; ptId < (numPoints+8); ptId++)
      {
      Mesh->GetPointCells(ptId, cells);
      for (i=0; i < cells.GetNumberOfIds(); i++)
        {
        triUse[cells.GetId(i)] = 0; //mark as deleted
        }
      }
    }
//
// If non-zero alpha value, then figure out which parts of mesh are
// contained within alpha radius.
//
  if ( this->Alpha > 0.0 )
    {
    float alpha2 = this->Alpha * this->Alpha;
    float x1[3], x2[3], x3[3];
    int j, cellId, numNei, p1, p2, nei;

    vtkCellArray *alphaVerts = new vtkCellArray(numPoints);
    vtkCellArray *alphaLines = new vtkCellArray(numPoints);

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
          for (j=0; j<3; j++) pointUse[triPts[j]] = 1; 
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

          if ( this->BoundingTriangulation || (p1 < numPoints && p2 < numPoints ) )
            {
            Mesh->GetCellEdgeNeighbors(cellId,p1,p2,neighbors);
            numNei = neighbors.GetNumberOfIds();

            if ( numNei < 1 || ((nei=neighbors.GetId(0)) > cellId && !triUse[nei]) )
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
      if ( !pointUse[ptId] &&  (ptId < numPoints || this->BoundingTriangulation) )
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
//
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

  if ( this->Alpha <= 0.0 && this->BoundingTriangulation )
    {
    output->SetPolys(triangles);
    }
  else
    {
    vtkCellArray *alphaTriangles = new vtkCellArray(numTriangles);
    int *triPts;

    for (i=0; i<numTriangles; i++)
      {
      if ( triUse[i] )
        {
        Mesh->GetCellPoints(i,npts,triPts);
        alphaTriangles->InsertNextCell(3,triPts);
        }
      }
    output->SetPolys(alphaTriangles);
    alphaTriangles->Delete();
    delete [] triUse;
    }

  points->Delete();
  triangles->Delete();
  delete Mesh;

  output->Squeeze();
}

void vtkDelaunay2D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetFilter::PrintSelf(os,indent);

  os << indent << "Alpha: " << this->Alpha << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Bounding Triangulation: " << (this->BoundingTriangulation ? "On\n" : "Off\n");
}
