/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelaunay3D.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkDelaunay3D.hh"
#include "vtkMath.hh"
#include "vtkTetra.hh"
#include "vtkTriangle.hh"

// Description:
// Construct object with Alpha = 0.0; Tolerance = 0.001; Offset = 1.25;
// BoundingTriangulation turned off.
vtkDelaunay3D::vtkDelaunay3D()
{
  this->Alpha = 0.0;
  this->Tolerance = 0.001;
  this->BoundingTriangulation = 0;
  this->Offset = 2.5;

  this->Output = new vtkUnstructuredGrid;
  this->Output->SetSource(this);
}

// Determine whether point x is inside of circumsphere of tetrahedron
// defined by points (x1, x2, x3, x4). Returns non-zero if inside sphere.
static int InSphere (float x[3], float x1[3], float x2[3], float x3[3],
                     float x4[3])
{
  static vtkMath math;
  static vtkTetra tetra;
  float radius2, center[3], dist2;

  radius2 = tetra.Circumsphere(x1,x2,x3,x4,center);

  // check if inside/outside circumcircle
  dist2 = (x[0]-center[0]) * (x[0]-center[0]) + 
          (x[1]-center[1]) * (x[1]-center[1]) +
          (x[2]-center[2]) * (x[2]-center[2]);

  if ( dist2 < (0.9999*radius2) ) return 1;
  else return 0;
}

static int NumberOfDuplicatePoints;

// Recursive method to locate tetrahedron containing point. Starts with
// arbitrary tetrahedron (tetra) and "walks" towards it. Influenced by 
// some of Guibas and Stolfi's work. Returns id of enclosing tetra, or -1 
// if no tetrahedron found.
static int FindTetra(float x[3], int ptIds[4], int tetra, 
                     vtkUnstructuredGrid *Mesh, 
                     vtkFloatPoints *points, float tol)
{
  int i, j, npts, inside, i2, i3, i4;
  vtkIdList pts(4), facePts(3);
  static vtkMath math;
  vtkIdList neighbors(2);
  float p[4][3], v12[3], vp[3], vx[3], v32[3], n[3], valx, valp;
  
  // get local tetrahedron info
  Mesh->GetCellPoints(tetra,pts);
  for (i=0; i<4; i++) 
    {
    ptIds[i] = pts.GetId(i);
    points->GetPoint(ptIds[i],p[i]);
    }

  // evaluate in/out of each face
  for (inside=1, i=0; i<4; i++)
    {
    i2 = (i+1) % 4;
    i3 = (i+2) % 4;
    i4 = (i+3) % 4;

    // compute normal and local vectors
    for (j=0; j<3; j++)
      {
      v32[j] = p[i3][j] - p[i2][j];
      v12[j] = p[i][j] - p[i2][j];
      vp[j] = p[i4][j] - p[i2][j];
      vx[j] = x[j] - p[i2][j];
      }

    if ( math.Normalize(vx) <= tol ) //check for duplicate point
      {
      NumberOfDuplicatePoints++;
      return -1;
      }

    if ( math.Normalize(vp) <= 1.0e-04 ) continue; //maybe on face
    math.Cross(v32,v12,n); math.Normalize(n);//face normal

    //see whether point and triangle vertex are on same side of tetra face
    valp = math.Dot(n,vp);
    if ( fabs(valx = math.Dot(n,vx)) <= 1.0e-04 ) continue; //maybe on face

    if ( (valx < 0.0 && valp > 0.0) || (valx > 0.0 && valp < 0.0)  )
      {
      inside = 0;
      facePts.SetId(0,ptIds[i]);
      facePts.SetId(1,ptIds[i2]);
      facePts.SetId(2,ptIds[i3]);
      Mesh->GetCellNeighbors(tetra, facePts, neighbors);
      if ( neighbors.GetNumberOfIds() > 0 ) //not boundary
        {
        return FindTetra(x,ptIds,neighbors.GetId(0),Mesh,points,tol);
        }
      }//outside this edge

    }//for each edge

  //must be in this tetraangle if all edges test inside
  if ( !inside ) return -1;
  else return tetra;
}

// Recursive method checks whether face is Delaunay, and if not, swaps face.
// Continues until all faces are Delaunay. Points p1,p2,p3 form the face in
// question; x is the coordinates of the inserted point; tetra is the current
// triangle id; Mesh is a pointer to cell structure.
static void CheckFace(int ptId, float x[3], int p1, int p2, int p3, int tetra, 
                      vtkUnstructuredGrid *Mesh, vtkFloatPoints *points)
{
  int i, numNei, nei, npts, *pts, p4, newTetra;
  float x1[3], x2[3], x3[3], x4[3];
  vtkIdList neighbors(2), facePts(3);
  int swapTetra[4];

  points->GetPoint(p1,x1);
  points->GetPoint(p2,x2);
  points->GetPoint(p3,x3);

  facePts.SetId(0,p1);
  facePts.SetId(1,p2);
  facePts.SetId(2,p3);

  Mesh->GetCellNeighbors(tetra,facePts,neighbors);
  numNei = neighbors.GetNumberOfIds();

  if ( numNei > 0 ) //i.e., not a boundary face
    {
    // get neighbor info including opposite point
    nei = neighbors.GetId(0);
    Mesh->GetCellPoints(nei, npts, pts);
    for (i=0; i<3; i++)
      if ( pts[i] != p1 && pts[i] != p2 && pts[i] != p3 )
        break;

    p4 = pts[i];
    points->GetPoint(p4,x3);

    // see whether point is in circumsphere
    if ( InSphere (x4, x, x1, x2, x3) )
      {// swap diagonal
      Mesh->RemoveReferenceToCell(p1,nei);
      Mesh->RemoveReferenceToCell(p3,tetra);

      Mesh->ResizeCellList(ptId,1);
      Mesh->AddReferenceToCell(ptId,nei);
      Mesh->ResizeCellList(p4,1);
      Mesh->AddReferenceToCell(p4,tetra);

      swapTetra[0] = ptId; swapTetra[1] = p4; 
      swapTetra[2] = p1; swapTetra[3] = p2;
      Mesh->ReplaceCell(tetra,4,swapTetra);

      swapTetra[2] = p2; swapTetra[3] = p3;
      Mesh->ReplaceCell(nei,4,swapTetra);

      swapTetra[2] = p3; swapTetra[3] = p1; 
      newTetra = Mesh->InsertNextLinkedCell(VTK_TETRA,4,swapTetra);

      // three new faces become suspect
      CheckFace(ptId, x, p1, p2, p4, tetra, Mesh, points);
      CheckFace(ptId, x, p2, p3, p4, nei, Mesh, points);
      CheckFace(ptId, x, p3, p1, p4, newTetra, Mesh, points);

      }//in circle
    }//interior edge
}

// 3D Delaunay triangulation. Steps are as follows:
//   1. For each point
//   2. Find triangle point is in
//   3. Create 3 triangles from each edge of triangle that point is in
//   4. Recursively evaluate Delaunay criterion for each edge neighbor
//   5. If criterion not satisfied; swap diagonal
// 
void vtkDelaunay3D::Execute()
{
  int numPoints, numTetras, i;
  int ptId, tetra[4];
  vtkPoints *inPoints;
  vtkFloatPoints *points;
  vtkUnstructuredGrid *Mesh=new vtkUnstructuredGrid;
  vtkPointSet *input=(vtkPointSet *)this->Input;
  vtkUnstructuredGrid *output=(vtkUnstructuredGrid *)this->Output;
  float x[3];
  int nodes[4][4], pts[4], npts, *tetraPts;
  vtkIdList neighbors(2), cells(64);
  float center[3], length, tol;
  vtkMath math;
  char *tetraUse;

  vtkDebugMacro(<<"Generating 3D Delaunay triangulation");
//
// Initialize; check input
//
  if ( (inPoints=input->GetPoints()) == NULL )
    {
    vtkErrorMacro("<<Cannot triangulate; no input points");
    return;
    }

  if ( (numPoints=inPoints->GetNumberOfPoints()) <= 0 )
    {
    vtkErrorMacro("<<Cannot triangulate; need at least 4 input points");
    return;
    }
  NumberOfDuplicatePoints = 0;
//
// Create initial bounding triangulation. Have to create bounding points.
// Initialize mesh structure.
//
  points = new vtkFloatPoints(numPoints+6);
  for (ptId=0; ptId < numPoints; ptId++)
    {
    points->SetPoint(ptId,inPoints->GetPoint(ptId));
    }

  input->GetCenter(center);
  tol = input->GetLength();
  if ( (length = this->Offset * tol) <= 0.0 ) length = 1.0;
  tol *= this->Tolerance;

  //create bounding octahedron
  x[0] = center[0] - length;
  x[1] = center[1];
  x[2] = center[2];
  points->SetPoint(numPoints,x);

  x[0] = center[0] + length;
  x[1] = center[1];
  x[2] = center[2];
  points->SetPoint(numPoints+1,x);

  x[0] = center[0];              
  x[1] = center[1] - length;
  x[2] = center[2];
  points->SetPoint(numPoints+2,x);

  x[0] = center[0];              
  x[1] = center[1] + length;
  x[2] = center[2];
  points->SetPoint(numPoints+3,x);

  x[0] = center[0];              
  x[1] = center[1];
  x[2] = center[2] - length;
  points->SetPoint(numPoints+4,x);

  x[0] = center[0];              
  x[1] = center[1];
  x[2] = center[2] + length;
  points->SetPoint(numPoints+5,x);

  Mesh->Allocate(5*numPoints);

  //create bounding tetras (there are six)
  pts[0] = numPoints + 4; pts[1] = numPoints + 5; 
  pts[2] = numPoints ; pts[3] = numPoints + 1;
  Mesh->InsertNextCell(VTK_TETRA,4,pts);

  pts[0] = numPoints + 4; pts[1] = numPoints + 5; 
  pts[2] = numPoints + 1; pts[3] = numPoints + 2;
  Mesh->InsertNextCell(VTK_TETRA,4,pts);

  pts[0] = numPoints + 4; pts[1] = numPoints + 5; 
  pts[2] = numPoints + 2; pts[3] = numPoints + 3;
  Mesh->InsertNextCell(VTK_TETRA,4,pts);

  pts[0] = numPoints + 4; pts[1] = numPoints + 5; 
  pts[2] = numPoints + 3; pts[3] = numPoints;
  tetra[1] = Mesh->InsertNextCell(VTK_TETRA,4,pts);

  Mesh->SetPoints(points);
  Mesh->BuildLinks();
//
// For each point; find tetra containing point. Then evaluate four
// neighboring tetras for Delaunay criterion. Tetras that do not 
// satisfy criterion have their faces swapped. This continues recursively 
// until all tetras have been shown to be Delaunay.
//
  for (ptId=0; ptId < numPoints; ptId++)
    {
    points->GetPoint(ptId,x);
    if ( (tetra[0] = FindTetra(x,pts,tetra[1],Mesh,points,tol)) >= 0 )
      {
      //delete this tetra; create four new tetras
      //first tetra is replaced with one of the new ones
      nodes[0][0] = ptId; nodes[0][1] = pts[0]; 
      nodes[0][2] = pts[1]; nodes[0][3] = pts[2];
      Mesh->RemoveReferenceToCell(pts[3], tetra[0]);
      Mesh->ReplaceCell(tetra[0], 4, nodes[0]);
      Mesh->ResizeCellList(ptId,1);
      Mesh->AddReferenceToCell(ptId,tetra[0]);

      //create three new tetras
      nodes[1][0] = ptId; nodes[1][1] = pts[1]; 
      nodes[1][2] = pts[2]; nodes[1][3] = pts[3];
      tetra[1] = Mesh->InsertNextLinkedCell(VTK_TETRA, 4, nodes[1]);

      nodes[2][0] = ptId; nodes[2][1] = pts[2]; 
      nodes[2][2] = pts[3]; nodes[2][3] = pts[0];
      tetra[2] = Mesh->InsertNextLinkedCell(VTK_TETRA, 4, nodes[2]);

      nodes[3][0] = ptId; nodes[3][1] = pts[3]; 
      nodes[3][2] = pts[0]; nodes[3][3] = pts[1];
      tetra[3] = Mesh->InsertNextLinkedCell(VTK_TETRA, 4, nodes[3]);

      // Check face neighbors for Delaunay criterion. If not satisfied, 
      // "swap" face. (This is done recursively.)
      CheckFace(ptId, x, pts[0], pts[1], pts[2], tetra[0], Mesh, points);
      CheckFace(ptId, x, pts[1], pts[2], pts[3], tetra[1], Mesh, points);
      CheckFace(ptId, x, pts[2], pts[3], pts[0], tetra[2], Mesh, points);
      CheckFace(ptId, x, pts[3], pts[0], pts[1], tetra[3], Mesh, points);

      }//if triangle found
    }//for all points

  vtkDebugMacro(<<"Triangulated " << numPoints <<" points, " 
                << NumberOfDuplicatePoints << " of which were duplicates");
//
// Finish up by deleting all tetras connected to initial triangulation
//
  numTetras = Mesh->GetNumberOfCells();
  tetraUse = new char[numTetras];
  for (i=0; i<numTetras; i++) tetraUse[i] = 1;

  if ( ! this->BoundingTriangulation )
    {
    for (ptId=numPoints; ptId < (numPoints+8); ptId++)
      {
      Mesh->GetPointCells(ptId, cells);
      for (i=0; i < cells.GetNumberOfIds(); i++)
        {
        tetraUse[cells.GetId(i)] = 0; //mark as deleted
        }
      }
    }
//
// If non-zero alpha value, then figure out which parts of mesh are
// contained within alpha radius.
//
  if ( this->Alpha > 0.0 )
    {
    char *pointUse = new char[numPoints+8];
    for (ptId=0; ptId < (numPoints+8); ptId++) pointUse[ptId] = 0;

    //traverse all points, create vertices if none used
    for (ptId=0; ptId<(numPoints+8); ptId++)
      {
      if ( !pointUse[ptId] &&  (ptId < numPoints || this->BoundingTriangulation) )
        {
        pts[0] = ptId;
        output->InsertNextCell(VTK_VERTEX,1,pts);
        }
      }

    // update output
    delete [] pointUse;
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

  output->Allocate(5*numPoints);
  for (i=0; i<numTetras; i++)
    {
    if ( tetraUse[i] )
      {
      Mesh->GetCellPoints(i,npts,tetraPts);
      output->InsertNextCell(VTK_TETRA,4,tetraPts);
      }
    }
  delete [] tetraUse;

  points->Delete();
  delete Mesh;

  output->Squeeze();
}

void vtkDelaunay3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSetFilter::PrintSelf(os,indent);

  os << indent << "Alpha: " << this->Alpha << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Bounding Triangulation: " << (this->BoundingTriangulation ? "On\n" : "Off\n");
}
