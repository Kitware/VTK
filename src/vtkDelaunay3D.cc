/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelaunay3D.cc
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
#include "vtkDelaunay3D.hh"
#include "vtkMath.hh"
#include "vtkTetra.hh"
#include "vtkTriangle.hh"
#include "vtkEdgeTable.hh"
#include "vtkPolyData.hh"

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
static int FindTetra(float x[3], int ptIds[4], float p[4][3], 
                     int tetra, vtkUnstructuredGrid *Mesh, 
                     vtkFloatPoints *points, float tol)
{
  int i, j, inside, i2, i3, i4;
  vtkIdList pts(4), facePts(3);
  vtkIdList neighbors(2);
  float v12[3], vp[3], vx[3], v32[3], n[3], valx, valp, maxValx;
  
  // get local tetrahedron info
  Mesh->GetCellPoints(tetra,pts);
  for (i=0; i<4; i++) 
    {
    ptIds[i] = pts.GetId(i);
    points->GetPoint(ptIds[i],p[i]);
    }

  // evaluate in/out of each face
  for (inside=1, maxValx=0.0, i=0; i<4; i++)
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

    if ( vtkMath::Normalize(vx) <= tol ) //check for duplicate point
      {
      NumberOfDuplicatePoints++;
      return -1;
      }

    if ( vtkMath::Normalize(vp) <= 1.0e-04 ) continue; //maybe on face
    vtkMath::Cross(v32,v12,n); vtkMath::Normalize(n);//face normal

    //see whether point and tetra vertex are on same side of tetra face
    valp = vtkMath::Dot(n,vp);
    if ( fabs(valx = vtkMath::Dot(n,vx)) <= 1.0e-04 ) return tetra;

    if ( (valx < 0.0 && valp > 0.0) || (valx > 0.0 && valp < 0.0)  )
      {
      if ( fabs(valx) > maxValx )
        {
        inside = 0;
        maxValx = fabs(valx);
        facePts.SetId(0,ptIds[i]);
        facePts.SetId(1,ptIds[i2]);
        facePts.SetId(2,ptIds[i3]);
        }
      }//outside this face
    }//for each face

  //must be in this tetra if all faces test inside
  if ( !inside ) 
    {
    Mesh->GetCellNeighbors(tetra, facePts, neighbors);
    return FindTetra(x,ptIds,p,neighbors.GetId(0),Mesh,points,tol);
    }
  else 
    {
    return tetra;
    }
}

// Find all faces that enclose a point. (Enclosure means not satifying 
// Delaunay criterion.) This method works in two distinct parts. First, the
// tetrhedra containing the point are found (there may be more than one if 
// the point falls on an edge or face). Next, face neighbors of these points
// are visited to see whether they satisfy the Delaunay criterion. Face 
// neighbors are visited repeatedly until no more tetrahedron are found.
// Enclosing tetras are returned in the tetras list; the enclosing faces 
// are returned in the faces list.
static int FindEnclosingFaces(float x[3], int tetra, vtkUnstructuredGrid *Mesh,
                              vtkFloatPoints *points, float tol,
                              vtkIdList &tetras, vtkIdList &faces,
                              vtkPointLocator &Locator)
{
  vtkTetra *tetraCell=NULL;
  int ptIds[4];
  float bcoords[4], *x1, *x2, *x3, *x4;
  float p[4][3];
  int tetraId=(-1), verts[4], onCount, i, j, numTetras;
  static vtkIdList boundaryPts(3), checkedTetras(25), neiTetras(2);
  int p1, p2, p3, insertFace;
  int npts, *tetraPts, nei, numNeiPts, *neiPts;
  int closestPoint;
  float closest[3], pcoords[3], dist2, weights[4];
  int subId;

  tetras.Reset();
  faces.Reset();
  boundaryPts.Reset();
  onCount = verts[0] = verts[1] = verts[2] = verts[3] = 0;

  // start off by finding closest point and tetras that use the point
  if ( Locator.IsInsertedPoint(x) >= 0 ) 
    {
    return 0;
    }

  closestPoint = Locator.FindClosestInsertedPoint(x);
  Mesh->GetPointCells(closestPoint, checkedTetras);

  //check these cells to see if any contain the point
  for ( i=0; i < checkedTetras.GetNumberOfIds(); i++ )
    {
    tetraId = checkedTetras.GetId(i);
    tetraCell = (vtkTetra *) Mesh->GetCell(tetraId);
    if ( tetraCell->EvaluatePosition(x,closest,subId,pcoords,dist2,weights) == 1 )
      {
      for ( i=0; i < 4; i++ ) 
        {
        tetraCell->Points.GetPoint(i,p[i]);
        ptIds[i] = tetraCell->PointIds.GetId(i);
        }
      break;
      }
    }
  if ( i >= checkedTetras.GetNumberOfIds() ) //keep looking for enclosing tetra
    {
    tetraId = FindTetra(x,ptIds,p,tetra,Mesh,points,tol);
    if ( tetraId < 0 ) return 0; 
    tetraCell = (vtkTetra *) Mesh->GetCell(tetraId);
    }

  if ( tetraCell->BarycentricCoords(x, p[0], p[1], p[2], p[3], bcoords) )
    {
    //check edges / faces for point being "on" them. Coincident points
    //should have been caught already.
    for (i=0; i < 4; i++ )
      {
      if ( bcoords[i] <= tol )
        {
        verts[i] = 1;
        onCount++;
        }
      }

    if ( onCount == 0 ) //inside tetra
      {
      tetras.InsertNextId(tetraId);
      }

    else if ( onCount == 1 ) //on face
      {
      for (i=0; i < 4; i++)
        if ( !verts[i] ) 
          boundaryPts.InsertNextId(ptIds[i]);

      Mesh->GetCellNeighbors(tetraId, boundaryPts, tetras);
      tetras.InsertNextId(tetraId);
      }

    else if ( onCount == 2 ) //on edge
      {
      for (i=0; i < 4; i++)
        if ( !verts[i] ) 
          boundaryPts.InsertNextId(ptIds[i]);

      Mesh->GetCellNeighbors(tetraId, boundaryPts, tetras);
      tetras.InsertNextId(tetraId);
      }

    else //on vertex - shouldn't happen, but hey, you never know!
      {
      NumberOfDuplicatePoints++;
      return 0;
      }

    }//if non-degenerate tetra

  // Okay, check face neighbors for Delaunay criterion. Purpose is to find 
  // list of enclosing faces and deleted tetras.
  numTetras = tetras.GetNumberOfIds();
  for (checkedTetras.Reset(), i=0; i < numTetras; i++) 
    {
    checkedTetras.InsertId(i,tetras.GetId(i));
    }

  for (i=0; i < numTetras; i++)
    {
    tetraId = tetras.GetId(i);
    Mesh->GetCellPoints(tetraId,npts,tetraPts);
    for (j=0; j < 4; j++)
      {
      boundaryPts.Reset(); neiTetras.Reset(); insertFace = 0;
      boundaryPts.InsertNextId((p1=tetraPts[j]));
      boundaryPts.InsertNextId((p2=tetraPts[(j+1)%4]));
      boundaryPts.InsertNextId((p3=tetraPts[(j+2)%4]));
      Mesh->GetCellNeighbors(tetraId, boundaryPts, neiTetras);

      //if a boundary face or an enclosing face
      if ( neiTetras.GetNumberOfIds() == 0 )
        {
        insertFace = 1;
        }
      else
        {
        nei = neiTetras.GetId(0);
        if ( ! checkedTetras.IsId(nei) )
          {
          Mesh->GetCellPoints(nei,numNeiPts,neiPts);
          x1 = points->GetPoint(neiPts[0]);
          x2 = points->GetPoint(neiPts[1]);
          x3 = points->GetPoint(neiPts[2]);
          x4 = points->GetPoint(neiPts[3]);
          if ( ! InSphere(x,x1,x2,x3,x4) ) //if point outside circumsphere
            {
            insertFace = 1;
            }
          else
            {
            numTetras++;
            tetras.InsertNextId(nei); //delete this tetra
            }
          checkedTetras.InsertNextId(nei);
          }
        else
          {
          if ( ! tetras.IsId(nei) ) //if checked but not deleted
            {
            insertFace = 1;
            }
          }
        }

      if ( insertFace )
        {
        faces.InsertNextId(p1);
        faces.InsertNextId(p2);
        faces.InsertNextId(p3);
        }

      }//for each tetra face
    }//for all deleted tetras

  // Okay, let's delete the tetras and prepare the data structure 
  for (i=0; i < tetras.GetNumberOfIds(); i++)
    {
    tetraId = tetras.GetId(i);
    Mesh->GetCellPoints(tetraId, npts, tetraPts);
    for (j=0; j<4; j++)
      {
      Mesh->RemoveReferenceToCell(tetraPts[j],tetraId);
      }
    }

  return (faces.GetNumberOfIds() / 3);
}

// 3D Delaunay triangulation. Steps are as follows:
//   1. For each point
//   2. Find tetrahedron point is in
//   3. Repeatedly visit face neighbors and evaluate Delaunay criterion 
//   4. Gather list of faces forming boundary of insertion polyhedron
//   5. Make sure that faces/point combination forms good tetrahedron
//   6. Create tetrahedron from each point/face combination
// 
void vtkDelaunay3D::Execute()
{
  int numPoints, numTetras, i;
  int tetraNum, numFaces, tetraId;
  int ptId;
  vtkPoints *inPoints;
  vtkFloatPoints *points;
  vtkUnstructuredGrid *Mesh=new vtkUnstructuredGrid;
  vtkPointSet *input=(vtkPointSet *)this->Input;
  vtkUnstructuredGrid *output=(vtkUnstructuredGrid *)this->Output;
  float x[3];
  int nodes[4], pts[4], npts, *tetraPts;
  vtkIdList neighbors(2), cells(64), tetras(5), faces(15);
  float center[3], length, tol;
  char *tetraUse;
  float bounds[6];
  
  vtkDebugMacro(<<"Generating 3D Delaunay triangulation");
//
// Initialize; check input
//
  if ( (inPoints=input->GetPoints()) == NULL )
    {
    vtkErrorMacro("<<Cannot triangulate; no input points");
    return;
    }

  numPoints = inPoints->GetNumberOfPoints();
  NumberOfDuplicatePoints = 0;
//
// Create initial bounding triangulation. Have to create bounding points.
// Initialize mesh structure.
//
  points = new vtkFloatPoints(numPoints+6);

  input->GetCenter(center);
  tol = input->GetLength();
  if ( (length = this->Offset * tol) <= 0.0 ) length = 1.0;
  tol *= this->Tolerance;

  bounds[0] = center[0] - length; bounds[1] = center[0] + length; 
  bounds[2] = center[1] - length; bounds[3] = center[1] + length; 
  bounds[4] = center[2] - length; bounds[5] = center[2] + length; 
  this->Locator.InitPointInsertion(points,bounds);

  //create bounding octahedron
  x[0] = center[0] - length;
  x[1] = center[1];
  x[2] = center[2];
  this->Locator.InsertPoint(numPoints,x);

  x[0] = center[0] + length;
  x[1] = center[1];
  x[2] = center[2];
  this->Locator.InsertPoint(numPoints+1,x);

  x[0] = center[0];              
  x[1] = center[1] - length;
  x[2] = center[2];
  this->Locator.InsertPoint(numPoints+2,x);

  x[0] = center[0];              
  x[1] = center[1] + length;
  x[2] = center[2];
  this->Locator.InsertPoint(numPoints+3,x);

  x[0] = center[0];              
  x[1] = center[1];
  x[2] = center[2] - length;
  this->Locator.InsertPoint(numPoints+4,x);

  x[0] = center[0];              
  x[1] = center[1];
  x[2] = center[2] + length;
  this->Locator.InsertPoint(numPoints+5,x);

  Mesh->Allocate(5*numPoints);

  //create bounding tetras (there are six)
  pts[0] = numPoints + 4; pts[1] = numPoints + 5; 
  pts[2] = numPoints ; pts[3] = numPoints + 2;
  Mesh->InsertNextCell(VTK_TETRA,4,pts);

  pts[0] = numPoints + 4; pts[1] = numPoints + 5; 
  pts[2] = numPoints + 2; pts[3] = numPoints + 1;
  Mesh->InsertNextCell(VTK_TETRA,4,pts);

  pts[0] = numPoints + 4; pts[1] = numPoints + 5; 
  pts[2] = numPoints + 1; pts[3] = numPoints + 3;
  Mesh->InsertNextCell(VTK_TETRA,4,pts);

  pts[0] = numPoints + 4; pts[1] = numPoints + 5; 
  pts[2] = numPoints + 3; pts[3] = numPoints;
  tetraId = Mesh->InsertNextCell(VTK_TETRA,4,pts);

  Mesh->SetPoints(points);
  Mesh->BuildLinks();
//
// For each point; find faces containing point. (Faces are found by deleting
// one or more tetrahedra "containing" point. Tetrahedron contain point when 
// they do not satisfy Delaunay criterion. (More than one tetra may contain
// a point if the point is on or near an edge or face.) For each face, create 
// a tetrahedron. (The locator helps speed search of points in tetras.)
  
  for (ptId=0; ptId < numPoints; ptId++)
    {
    inPoints->GetPoint(ptId,x);

    if ( (numFaces=FindEnclosingFaces(x,tetraId,Mesh,
    points,this->Tolerance,tetras,faces,this->Locator)) > 0 )
      {
      this->Locator.InsertPoint(ptId,x); //point is part of mesh now

      for (tetraNum=0; tetraNum < numFaces; tetraNum++)
        {
        //create tetrahedron
        nodes[0] = ptId;
        nodes[1] = faces.GetId(3*tetraNum);
        nodes[2] = faces.GetId(3*tetraNum+1);
        nodes[3] = faces.GetId(3*tetraNum+2);

        //either replace previously deleted tetra or create new one
        if ( tetraNum < tetras.GetNumberOfIds() )
          {
          tetraId = tetras.GetId(tetraNum);
          Mesh->ReplaceCell(tetraId, 4, nodes);
          for (i=0; i < 4; i++)
            {
            Mesh->ResizeCellList(nodes[i],1);
            Mesh->AddReferenceToCell(nodes[i],tetraId);
            }
          }
        else
          {
          tetraId = Mesh->InsertNextLinkedCell(VTK_TETRA, 4, nodes);
          }

        }//for each face
      }//if enclosing faces found
    }//for all points

  vtkDebugMacro(<<"Triangulated " << numPoints <<" points, " 
                << NumberOfDuplicatePoints << " of which were duplicates");
//
// Send appropriate portions of triangulation to output
//
  output->Allocate(5*numPoints);
  numTetras = Mesh->GetNumberOfCells();
  tetraUse = new char[numTetras];
  for (i=0; i < numTetras; i++) tetraUse[i] = 1;

  //if boundary triangulation not desired, delete tetras connected to 
  // boundary points
  if ( ! this->BoundingTriangulation )
    {
    for (ptId=numPoints; ptId < (numPoints+6); ptId++)
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
    float alpha2 = this->Alpha * this->Alpha;
    vtkEdgeTable edges(numPoints+6);
    char *pointUse = new char[numPoints+6];
    int p1, p2, p3, nei, j, k, numNei;
    float x1[3], x2[3], x3[3], x4[3], v1[2], v2[2], v3[2];
    static int edge[6][2] = {{0,1},{1,2},{2,0},{0,3},{1,3},{2,3}};
    static vtkIdList boundaryPts(3), neiTetras(2);
    static vtkTriangle tri;
    static vtkTetra tetra;

    for (ptId=0; ptId < (numPoints+6); ptId++) pointUse[ptId] = 0;

    //traverse all tetras, checking against alpha radius
    for (i=0; i < numTetras; i++)
      {
      //check tetras
      if ( tetraUse[i] == 1 )
        {
        Mesh->GetCellPoints(i, npts, tetraPts);
        points->GetPoint(tetraPts[0],x1);
        points->GetPoint(tetraPts[1],x2);
        points->GetPoint(tetraPts[2],x3);
        points->GetPoint(tetraPts[3],x4);

        //check tetras
        if ( tetra.Circumsphere(x1,x2,x3,x4,center) > alpha2 )
          {
          tetraUse[i] = 0;
          }
        else
          {
          for (j=0; j<4; j++) pointUse[tetraPts[j]] = 1; 
          for (j=0; j<6; j++)
            {
            p1 = tetraPts[edge[j][0]];
            p2 = tetraPts[edge[j][1]];
            if ( ! edges.IsEdge(p1,p2) ) edges.InsertEdge(p1,p2);
            }
          }
        }//if non-deleted tetra
      }//for all tetras

    //traverse tetras again, this time examining faces
    for (i=0; i < numTetras; i++)
      {
      if ( ! tetraUse[i] )
        {
        Mesh->GetCellPoints(i, npts, tetraPts);
        for (j=0; j < 4; j++)
          {
          p1 = tetraPts[j];
          p2 = tetraPts[(j+1)%4];
          p3 = tetraPts[(j+2)%4];

          //make sure face is okay to create
          if ( this->BoundingTriangulation || 
          (p1 < numPoints && p2 < numPoints && p3 < numPoints) )
            {
            boundaryPts.InsertId(0,p1);
            boundaryPts.InsertId(1,p2);
            boundaryPts.InsertId(2,p3);
            Mesh->GetCellNeighbors(tetraId, boundaryPts, neiTetras);
            numNei = neiTetras.GetNumberOfIds();

            if ( neiTetras.GetNumberOfIds() < 1  ||
            ( (nei = neiTetras.GetId(0)) > i && !tetraUse[nei] ) )
              {
              points->GetPoint(p1,x1); 
              points->GetPoint(p2,x2);
              points->GetPoint(p3,x3);
              tri.ProjectTo2D(x1,x2,x3,v1,v2,v3);
              if ( tri.Circumcircle(v1,v2,v3,center) <= alpha2 )
                {
                pts[0] = p1;
                pts[1] = p2;
                pts[2] = p3;
                output->InsertNextCell(VTK_TRIANGLE,3,pts);
                if ( ! edges.IsEdge(p1,p2) ) edges.InsertEdge(p1,p2);
                if ( ! edges.IsEdge(p2,p3) ) edges.InsertEdge(p2,p3);
                if ( ! edges.IsEdge(p3,p1) ) edges.InsertEdge(p3,p1);
                for (k=0; k<3; k++) pointUse[pts[k]] = 1; 
                }
              }//if candidate face
            }//if not boundary face or boundary faces requested
          }//if tetra isn't being output
        }//if tetra not output
      }//for all tetras

    //traverse tetras again, this time examining edges
    for (i=0; i < numTetras; i++)
      {
      if ( ! tetraUse[i] )
        {
        Mesh->GetCellPoints(i, npts, tetraPts);

        for (j=0; j < 6; j++)
          {
          p1 = tetraPts[edge[j][0]];
          p2 = tetraPts[edge[j][1]];

          if ( (this->BoundingTriangulation || (p1 < numPoints && p2 < numPoints))
          && (!edges.IsEdge(p1,p2)) )
            {
            points->GetPoint(p1,x1);
            points->GetPoint(p2,x2);
            if ( (vtkMath::Distance2BetweenPoints(x1,x2)*0.25) <= alpha2 )
              {
              edges.InsertEdge(p1,p2);
              pts[0] = p1;
              pts[1] = p2;
              output->InsertNextCell(VTK_LINE,2,pts);
              pointUse[p1] = 1; pointUse[p2] = 1; 
              }
            }//if edge a candidate
          }//for all edges of tetra
        }//if tetra not output
      }//for all tetras

    //traverse all points, create vertices if none used
    for (ptId=0; ptId<(numPoints+6); ptId++)
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

  for (i=0; i<numTetras; i++)
    {
    if ( tetraUse[i] )
      {
      Mesh->GetCellPoints(i,npts,tetraPts);
      output->InsertNextCell(VTK_TETRA,4,tetraPts);
      }
    }
  vtkDebugMacro(<<"Generated " << output->GetNumberOfPoints() << " points and "
                << output->GetNumberOfCells() << " tetrahedra");

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
