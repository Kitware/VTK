/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelaunay2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDelaunay2D.h"

#include "vtkAbstractTransform.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangle.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkDelaunay2D);
vtkCxxSetObjectMacro(vtkDelaunay2D,Transform,vtkAbstractTransform);

// Construct object with Alpha = 0.0; Tolerance = 0.00001; Offset = 1.25;
// BoundingTriangulation turned off.
vtkDelaunay2D::vtkDelaunay2D()
{
  this->Alpha = 0.0;
  this->Tolerance = 0.00001;
  this->BoundingTriangulation = 0;
  this->Offset = 1.0;
  this->Transform = NULL;
  this->ProjectionPlaneMode = VTK_DELAUNAY_XY_PLANE;

  // optional 2nd input
  this->SetNumberOfInputPorts(2);
}

vtkDelaunay2D::~vtkDelaunay2D()
{
  if (this->Transform)
    {
    this->Transform->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
// Specify the input data or filter. Old style.
void vtkDelaunay2D::SetSourceData(vtkPolyData *input)
{
  this->Superclass::SetInputData(1, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter. New style.
void vtkDelaunay2D::SetSourceConnection(vtkAlgorithmOutput *algOutput)
{
  this->Superclass::SetInputConnection(1, algOutput);
}

vtkPolyData *vtkDelaunay2D::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

// Determine whether point x is inside of circumcircle of triangle
// defined by points (x1, x2, x3). Returns non-zero if inside circle.
// (Note that z-component is ignored.)
int vtkDelaunay2D::InCircle (double x[3], double x1[3], double x2[3],
                             double x3[3])
{
  double radius2, center[2], dist2;

  radius2 = vtkTriangle::Circumcircle(x1,x2,x3,center);

  // check if inside/outside circumcircle
  dist2 = (x[0]-center[0]) * (x[0]-center[0]) +
          (x[1]-center[1]) * (x[1]-center[1]);

  if ( dist2 < (0.999999999999*radius2) )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

#define VTK_DEL2D_TOLERANCE 1.0e-014

// Recursive method to locate triangle containing point. Starts with arbitrary
// triangle (tri) and "walks" towards it. Influenced by some of Guibas and
// Stolfi's work. Returns id of enclosing triangle, or -1 if no triangle
// found. Also, the array nei[3] is used to communicate info about points
// that lie on triangle edges: nei[0] is neighboring triangle id, and nei[1]
// and nei[2] are the vertices defining the edge.
vtkIdType vtkDelaunay2D::FindTriangle(double x[3], vtkIdType ptIds[3],
                                      vtkIdType tri, double tol,
                                      vtkIdType nei[3], vtkIdList *neighbors)
{
  int i, j, ir, ic, inside, i2, i3;
  vtkIdType *pts, npts, newNei;
  double p[3][3], n[2], vp[2], vx[2], dp, minProj;

  // get local triangle info
  this->Mesh->GetCellPoints(tri,npts,pts);
  for (i=0; i<3; i++)
    {
    ptIds[i] = pts[i];
    this->GetPoint(ptIds[i], p[i]);
    }

  // Randomization (of find edge neighbora) avoids walking in
  // circles in certain weird cases
  srand(tri);
  ir = rand() % 3;
  // evaluate in/out of each edge
  for (inside=1, minProj=0.0, ic=0; ic<3; ic++)
    {
    i  = (ir+ic) % 3;
    i2 = (i+1) % 3;
    i3 = (i+2) % 3;

    // create a 2D edge normal to define a "half-space"; evaluate points (i.e.,
    // candidate point and other triangle vertex not on this edge).
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
    dp = vtkMath::Dot2D(n,vx) * (vtkMath::Dot2D(n,vp) < 0 ? -1.0 : 1.0);
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

  if ( inside ) // all edges have tested positive
    {
    nei[0] = (-1);
    return tri;
    }

  else if ( !inside && (fabs(minProj) < VTK_DEL2D_TOLERANCE) ) // on edge
    {
    this->Mesh->GetCellEdgeNeighbors(tri,nei[1],nei[2],neighbors);
    nei[0] = neighbors->GetId(0);
    return tri;
    }

  else //walk towards point
    {
    this->Mesh->GetCellEdgeNeighbors(tri,nei[1],nei[2],neighbors);
    if ( (newNei=neighbors->GetId(0)) == nei[0] )
      {
      this->NumberOfDegeneracies++;
      return -1;
      }
    else
      {
      nei[0] = tri;
      return this->FindTriangle(x,ptIds,newNei,tol,nei,neighbors);
      }
    }
}

#undef VTK_DEL2D_TOLERANCE

// Recursive method checks whether edge is Delaunay, and if not, swaps edge.
// Continues until all edges are Delaunay. Points p1 and p2 form the edge in
// question; x is the coordinates of the inserted point; tri is the current
// triangle id.
void vtkDelaunay2D::CheckEdge(vtkIdType ptId, double x[3], vtkIdType p1,
                              vtkIdType p2, vtkIdType tri)
{
  int i;
  vtkIdType *pts, npts, numNei, nei, p3;
  double x1[3], x2[3], x3[3];
  vtkIdList *neighbors;
  vtkIdType swapTri[3];

  this->GetPoint(p1,x1);
  this->GetPoint(p2,x2);

  neighbors = vtkIdList::New();
  neighbors->Allocate(2);

  this->Mesh->GetCellEdgeNeighbors(tri,p1,p2,neighbors);
  numNei = neighbors->GetNumberOfIds();

  if ( numNei > 0 ) //i.e., not a boundary edge
    {
    // get neighbor info including opposite point
    nei = neighbors->GetId(0);
    this->Mesh->GetCellPoints(nei, npts, pts);
    for (i=0; i<2; i++)
      {
      if ( pts[i] != p1 && pts[i] != p2 )
        {
        break;
        }
      }
    p3 = pts[i];
    this->GetPoint(p3,x3);

    // see whether point is in circumcircle
    if ( this->InCircle (x3, x, x1, x2) )
      {// swap diagonal
      this->Mesh->RemoveReferenceToCell(p1,tri);
      this->Mesh->RemoveReferenceToCell(p2,nei);
      this->Mesh->ResizeCellList(ptId,1);
      this->Mesh->AddReferenceToCell(ptId,nei);
      this->Mesh->ResizeCellList(p3,1);
      this->Mesh->AddReferenceToCell(p3,tri);

      swapTri[0] = ptId; swapTri[1] = p3; swapTri[2] = p2;
      this->Mesh->ReplaceCell(tri,3,swapTri);

      swapTri[0] = ptId; swapTri[1] = p1; swapTri[2] = p3;
      this->Mesh->ReplaceCell(nei,3,swapTri);

      // two new edges become suspect
      this->CheckEdge(ptId, x, p3, p2, tri);
      this->CheckEdge(ptId, x, p1, p3, nei);

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
int vtkDelaunay2D::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *source = 0;
  if (sourceInfo)
    {
    source =
      vtkPolyData::SafeDownCast(sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
    }
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPoints, i;
  vtkIdType numTriangles = 0;
  vtkIdType ptId, tri[4], nei[3];
  vtkIdType p1 = 0;
  vtkIdType p2 = 0;
  vtkIdType p3 = 0;
  vtkPoints *inPoints;
  vtkPoints *points;
  vtkPoints *tPoints = NULL;
  vtkCellArray *triangles;
  int ncells;
  vtkIdType nodes[4][3], *neiPts;
  vtkIdType *triPts = 0;
  vtkIdType numNeiPts;
  vtkIdType npts = 0;
  vtkIdType pts[3], swapPts[3];
  vtkIdList *neighbors, *cells;
  vtkIdType tri1, tri2;
  double center[3], radius, tol, x[3];
  double n1[3], n2[3];
  int *triUse = NULL;
  double *bounds;

  vtkDebugMacro(<<"Generating 2D Delaunay triangulation");

  if (this->Transform && this->BoundingTriangulation)
    {
    vtkWarningMacro(<<"Bounding triangulation cannot be used when an input transform is specified.  Output will not contain bounding triangulation.");
    }

  if (this->ProjectionPlaneMode == VTK_BEST_FITTING_PLANE && this->BoundingTriangulation)
    {
    vtkWarningMacro(<<"Bounding triangulation cannot be used when the best fitting plane option is on.  Output will not contain bounding triangulation.");
    }

  // Initialize; check input
  //
  if ( (inPoints=input->GetPoints()) == NULL )
    {
    vtkDebugMacro("Cannot triangulate; no input points");
    return 1;
    }

  if ( (numPoints=inPoints->GetNumberOfPoints()) <= 2 )
    {
    vtkDebugMacro("Cannot triangulate; need at least 3 input points");
    return 1;
    }

  neighbors = vtkIdList::New(); neighbors->Allocate(2);
  cells = vtkIdList::New(); cells->Allocate(64);

  this->NumberOfDuplicatePoints = 0;
  this->NumberOfDegeneracies = 0;

  this->Mesh = vtkPolyData::New();


  // If the user specified a transform, apply it to the input data.
  //
  // Only the input points are transformed.  We do not bother
  // transforming the source points (if specified).  The reason is
  // that only the topology of the Source is used during the constrain
  // operation.  The point ids in the Source topology are assumed to
  // reference points in the input. So, when an input transform is
  // used, only the input points are transformed.  We do not bother
  // with transforming the Source points since they are never
  // referenced.
  if (this->Transform)
    {
    tPoints = vtkPoints::New();
    this->Transform->TransformPoints(inPoints, tPoints);
    }
  else
    {
    // If the user asked this filter to compute the best fitting plane,
    // proceed to compute the plane and generate a transform that will
    // map the input points into that plane.
    if(this->ProjectionPlaneMode == VTK_BEST_FITTING_PLANE)
      {
      this->SetTransform( this->ComputeBestFittingPlane(input) );
      tPoints = vtkPoints::New();
      this->Transform->TransformPoints(inPoints, tPoints);
      }
    }

  // Create initial bounding triangulation. Have to create bounding points.
  // Initialize mesh structure.
  //
  points = vtkPoints::New();
  // This will copy doubles to doubles if the input is double.
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numPoints);
  if (!this->Transform)
    {
    points->DeepCopy(inPoints);
    }
  else
    {
    points->DeepCopy(tPoints);
    tPoints->Delete();
    tPoints = NULL;
    }

  bounds = points->GetBounds();
  center[0] = (bounds[0]+bounds[1])/2.0;
  center[1] = (bounds[2]+bounds[3])/2.0;
  center[2] = (bounds[4]+bounds[5])/2.0;
  tol = input->GetLength();
  radius = this->Offset * tol;
  tol *= this->Tolerance;

  for (ptId=0; ptId<8; ptId++)
    {
    x[0] = center[0]
      + radius*cos( ptId * vtkMath::RadiansFromDegrees( 45.0 ) );
    x[1] = center[1]
      + radius*sin( ptId * vtkMath::RadiansFromDegrees( 45.0 ) );
    x[2] = center[2];
    points->InsertPoint( numPoints + ptId, x );
    }
  // We do this for speed accessing points
  this->Points =
    static_cast<vtkDoubleArray *>(points->GetData())->GetPointer(0);

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
  tri[0] = 0;

  this->Mesh->SetPoints(points);
  this->Mesh->SetPolys(triangles);
  this->Mesh->BuildLinks(); //build cell structure

  // For each point; find triangle containing point. Then evaluate three
  // neighboring triangles for Delaunay criterion. Triangles that do not
  // satisfy criterion have their edges swapped. This continues recursively
  // until all triangles have been shown to be Delaunay.
  //
  for (ptId=0; ptId < numPoints; ptId++)
    {
    this->GetPoint(ptId,x);
    nei[0] = (-1); //where we are coming from...nowhere initially

    if ( (tri[0] = this->FindTriangle(x,pts,tri[0],tol,nei,neighbors)) >= 0 )
      {
      if ( nei[0] < 0 ) //in triangle
        {
        //delete this triangle; create three new triangles
        //first triangle is replaced with one of the new ones
        nodes[0][0] = ptId; nodes[0][1] = pts[0]; nodes[0][2] = pts[1];
        this->Mesh->RemoveReferenceToCell(pts[2], tri[0]);
        this->Mesh->ReplaceCell(tri[0], 3, nodes[0]);
        this->Mesh->ResizeCellList(ptId,1);
        this->Mesh->AddReferenceToCell(ptId,tri[0]);

        //create two new triangles
        nodes[1][0] = ptId; nodes[1][1] = pts[1]; nodes[1][2] = pts[2];
        tri[1] = this->Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[1]);

        nodes[2][0] = ptId; nodes[2][1] = pts[2]; nodes[2][2] = pts[0];
        tri[2] = this->Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[2]);

        // Check edge neighbors for Delaunay criterion. If not satisfied, flip
        // edge diagonal. (This is done recursively.)
        this->CheckEdge(ptId, x, pts[0], pts[1], tri[0]);
        this->CheckEdge(ptId, x, pts[1], pts[2], tri[1]);
        this->CheckEdge(ptId, x, pts[2], pts[0], tri[2]);
        }

      else // on triangle edge
        {
        //update cell list
        this->Mesh->GetCellPoints(nei[0],numNeiPts,neiPts);
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
        this->Mesh->ResizeCellList(p1,1);
        this->Mesh->ResizeCellList(p2,1);

        //replace two triangles
        this->Mesh->RemoveReferenceToCell(nei[2],tri[0]);
        this->Mesh->RemoveReferenceToCell(nei[2],nei[0]);
        nodes[0][0] = ptId; nodes[0][1] = p2; nodes[0][2] = nei[1];
        this->Mesh->ReplaceCell(tri[0], 3, nodes[0]);
        nodes[1][0] = ptId; nodes[1][1] = p1; nodes[1][2] = nei[1];
        this->Mesh->ReplaceCell(nei[0], 3, nodes[1]);
        this->Mesh->ResizeCellList(ptId, 2);
        this->Mesh->AddReferenceToCell(ptId,tri[0]);
        this->Mesh->AddReferenceToCell(ptId,nei[0]);

        tri[1] = nei[0];

        //create two new triangles
        nodes[2][0] = ptId; nodes[2][1] = p2; nodes[2][2] = nei[2];
        tri[2] = this->Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[2]);

        nodes[3][0] = ptId; nodes[3][1] = p1; nodes[3][2] = nei[2];
        tri[3] = this->Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[3]);

        // Check edge neighbors for Delaunay criterion.
        for ( i=0; i<4; i++ )
          {
          this->CheckEdge (ptId, x, nodes[i][1], nodes[i][2], tri[i]);
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
      this->UpdateProgress (static_cast<double>(ptId)/numPoints);
      if (this->GetAbortExecute())
        {
        break;
        }
      }

    }//for all points

  vtkDebugMacro(<<"Triangulated " << numPoints <<" points, "
                << this->NumberOfDuplicatePoints
                << " of which were duplicates");

  if ( this->NumberOfDegeneracies > 0 )
    {
    vtkDebugMacro(<< this->NumberOfDegeneracies
    << " degenerate triangles encountered, mesh quality suspect");
    }

  // Finish up by recovering the boundary, or deleting all triangles connected
  // to the bounding triangulation points or not satisfying alpha criterion,
  if ( !this->BoundingTriangulation || this->Alpha > 0.0 || source )
    {
    numTriangles = this->Mesh->GetNumberOfCells();
    if ( source )
      {
      triUse = this->RecoverBoundary(source);
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
      this->Mesh->GetPointCells(ptId, cells);
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
    double alpha2 = this->Alpha * this->Alpha;
    double x1[3], x2[3], x3[3];
    double xx1[3], xx2[3], xx3[3];
    vtkIdType cellId, numNei, ap1, ap2, neighbor;

    vtkCellArray *alphaVerts = vtkCellArray::New();
    alphaVerts->Allocate(numPoints);
    vtkCellArray *alphaLines = vtkCellArray::New();
    alphaLines->Allocate(numPoints);

    char *pointUse = new char[numPoints+8];
    for (ptId=0; ptId < (numPoints+8); ptId++)
      {
      pointUse[ptId] = 0;
      }

    //traverse all triangles; evaluating Delaunay criterion
    for (i=0; i < numTriangles; i++)
      {
      if ( triUse[i] == 1 )
        {
        this->Mesh->GetCellPoints(i, npts, triPts);

        // if any point is one of the bounding points that was added
        // at the beginning of the algorithm, then grab the points
        // from the variable "points" (this list has the boundary
        // points and the original points have been transformed by the
        // input transform).  if none of the points are bounding points,
        // then grab the points from the variable "inPoints" so the alpha
        // criterion is applied in the nontransformed space.
        if (triPts[0]<numPoints && triPts[1]<numPoints && triPts[2]<numPoints)
          {
          inPoints->GetPoint(triPts[0],x1);
          inPoints->GetPoint(triPts[1],x2);
          inPoints->GetPoint(triPts[2],x3);
          }
        else
          {
          points->GetPoint(triPts[0],x1);
          points->GetPoint(triPts[1],x2);
          points->GetPoint(triPts[2],x3);
          }

        // evaluate the alpha criterion in 3D
        vtkTriangle::ProjectTo2D(x1, x2, x3, xx1, xx2, xx3);
        if ( vtkTriangle::Circumcircle(xx1,xx2,xx3,center) > alpha2 )
          {
          triUse[i] = 0;
          }
        else
          {
          for (int j=0; j<3; j++)
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
          ap1 = triPts[i];
          ap2 = triPts[(i+1)%npts];

          if (this->BoundingTriangulation || (ap1<numPoints && ap2<numPoints))
            {
            this->Mesh->GetCellEdgeNeighbors(cellId,ap1,ap2,neighbors);
            numNei = neighbors->GetNumberOfIds();

            if ( numNei < 1 || ((neighbor=neighbors->GetId(0)) > cellId
                                && !triUse[neighbor]) )
              {//see whether edge is shorter than Alpha

              // same argument as above, if one is a boundary point, get
              // it using this->GetPoint() which are transformed points. if
              // neither of the points are boundary points, get the from
              // inPoints (untransformed points) so alpha comparison done
              // untransformed space
              if (ap1 < numPoints && ap2 < numPoints)
                {
                inPoints->GetPoint(ap1,x1);
                inPoints->GetPoint(ap2,x2);
                }
              else
                {
                this->GetPoint(ap1,x1);
                this->GetPoint(ap2,x2);
                }
              if ( (vtkMath::Distance2BetweenPoints(x1,x2)*0.25) <= alpha2 )
                {
                pointUse[ap1] = 1; pointUse[ap2] = 1;
                pts[0] = ap1;
                pts[1] = ap2;
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

  // The code below fixes a bug reported by Gilles Rougeron.
  // Some input points were not connected in the output triangulation.
  // The cause was that those points were only connected to triangles
  // scheduled for removal (i.e. triangles connected to the boundary).
  //
  // We wrote the following fix: swap edges so the unconnected points
  // become connected to new triangles not scheduled for removal.
  // We only applies if:
  // - the bounding triangulation must be deleted
  //   (BoundingTriangulation == OFF)
  // - alpha spheres are not used (Alpha == 0.0)
  // - the triangulation is not constrained (source == NULL)

  if ( !this->BoundingTriangulation && this->Alpha == 0.0 && !source )
    {
    bool isConnected;
    vtkIdType numSwaps = 0;

    for (ptId=0; ptId < numPoints; ptId++)
      {
      // check if point is only connected to triangles scheduled for
      // removal
      this->Mesh->GetPointCells(ptId, cells);
      ncells = cells->GetNumberOfIds();

      isConnected = false;

      for (i=0; i < ncells; i++)
        {
        if( triUse[cells->GetId(i)] )
          {
          isConnected = true;
          break;
          }
        }

      // this point will be connected in the output
      if( isConnected )
        {
        // point is connected: continue
        continue;
        }

      // This point is only connected to triangles scheduled for removal.
      // Therefore it will not be connected in the output triangulation.
      // Let's swap edges to create a triangle with 3 inner points.
      // - inner points have an id < numPoints
      // - boundary point ids are, numPoints <= id < numPoints+8.

      // visit every edge connected to that point.
      // check the 2 triangles touching at that edge.
      // if one triangle is connected to 2 non-boundary points

      for (i=0; i < ncells; i++)
        {
        tri1 = cells->GetId(i);
        this->Mesh->GetCellPoints(tri1,npts,triPts);

        if(triPts[0] == ptId)
          {
          p1 = triPts[1];
          p2 = triPts[2];
          }
        else if(triPts[1] == ptId)
          {
          p1 = triPts[2];
          p2 = triPts[0];
          }
        else
          {
          p1 = triPts[0];
          p2 = triPts[1];
          }

        // if both p1 & p2 are boundary points,
        // we skip them.
        if( p1 >= numPoints && p2 >= numPoints )
          {
          continue;
          }

        vtkDebugMacro( "tri " << tri1 << " [" << triPts[0]
          << " " << triPts[1] << " " << triPts[2] << "]" );

        vtkDebugMacro( "edge [" << p1 << " " << p2
          << "] non-boundary" );

        // get the triangle sharing edge [p1 p2] with tri1
        this->Mesh->GetCellEdgeNeighbors(tri1,p1,p2,neighbors);

        // Since p1 or p2 is not on the boundary,
        // the neighbor triangle should exist.
        // If more than one neighbor triangle exist,
        // the edge is non-manifold.
        if( neighbors->GetNumberOfIds() != 1 )
          {
          vtkErrorMacro("ERROR: Edge [" << p1 << " " << p2
              << "] is non-manifold!!!");
          return 0;
          }

        tri2 = neighbors->GetId(0);

        // get the 3 points of the neighbor triangle
        this->Mesh->GetCellPoints(tri2,npts,neiPts);

        vtkDebugMacro("triangle " << tri2 << " [" << neiPts[0] << " "
            << neiPts[1] << " " << neiPts[2] << "]" );

        // locate the point different from p1 and p2
        if( neiPts[0] != p1 && neiPts[0] != p2 )
          {
          p3 = neiPts[0];
          }
        else if( neiPts[1] != p1 && neiPts[1] != p2 )
          {
          p3 = neiPts[1];
          }
        else
          {
          p3 = neiPts[2];
          }

        vtkDebugMacro( "swap [" << p1 << " " << p2 << "] and ["
          << ptId << " " << p3 << "]" );

        // create the two new triangles.
        // we just need to replace their pt ids.
        pts[0] = ptId; pts[1] = p1; pts[2] = p3;
        swapPts[0] = ptId; swapPts[1] = p3; swapPts[2] = p2;

        vtkDebugMacro("candidate tri1 " << tri1 << " ["
          << pts[0] << " " << pts[1] << " " << pts[2] << "]"
          << " triUse " << triUse[tri1] );

        vtkDebugMacro("candidate tri2 " << tri2 << " ["
          << swapPts[0] << " " << swapPts[1] << " " << swapPts[2] << "]"
          << "triUse " << triUse[tri2] );

        // compute the normal for the 2 candidate triangles
        vtkTriangle::ComputeNormal(points,3,pts,n1);
        vtkTriangle::ComputeNormal(points,3,swapPts,n2);

        // the normals must be along the same direction,
        // or one triangle is upside down.
        if( vtkMath::Dot(n1,n2) < 0.0 )
          {
          // do not swap diagonal
          continue;
          }

        // swap edge [p1 p2] and diagonal [ptId p3]
        this->Mesh->RemoveReferenceToCell(p1,tri2);
        this->Mesh->RemoveReferenceToCell(p2,tri1);
        this->Mesh->ResizeCellList(ptId,1);
        this->Mesh->ResizeCellList(p3,1);
        this->Mesh->AddReferenceToCell(ptId,tri2);
        this->Mesh->AddReferenceToCell(p3,tri1);

        // it's ok to swap the diagonal
        this->Mesh->ReplaceCell(tri1,3,pts);
        this->Mesh->ReplaceCell(tri2,3,swapPts);

        triUse[tri1] = (p1 < numPoints && p3 < numPoints);
        triUse[tri2] = (p3 < numPoints && p2 < numPoints);

        vtkDebugMacro("replace tri1 " << tri1 << " [" << pts[0] << " "
            << pts[1] << " " << pts[2] << "]" << " triUse "
            << triUse[tri1] );

        vtkDebugMacro("replace tri2 " << tri2 << " ["
          << swapPts[0] << " " << swapPts[1] << " " << swapPts[2] << "]"
          << " triUse " << triUse[tri2] );

        // update the 'scheduled for removal' flag of the first triangle.
        // The second triangle was not scheduled for removal anyway.
        numSwaps++;
        vtkDebugMacro("numSwaps " << numSwaps );
        }
      }
    vtkDebugMacro("numSwaps " << numSwaps );
    }

  // Update output; free up supporting data structures.
  //
  if (this->BoundingTriangulation && !this->Transform)
    {
    output->SetPoints(points);
    }
  else
    {
    output->SetPoints(inPoints);
    output->GetPointData()->PassData(input->GetPointData());
    }

  if ( this->Alpha <= 0.0 && this->BoundingTriangulation && !source )
    {
    output->SetPolys(triangles);
    }
  else
    {
    vtkCellArray *alphaTriangles = vtkCellArray::New();
    alphaTriangles->Allocate(numTriangles);
    vtkIdType *alphaTriPts;

    for (i=0; i<numTriangles; i++)
      {
      if ( triUse[i] )
        {
        this->Mesh->GetCellPoints(i,npts,alphaTriPts);
        alphaTriangles->InsertNextCell(3,alphaTriPts);
        }
      }
    output->SetPolys(alphaTriangles);
    alphaTriangles->Delete();
    delete [] triUse;
    }

  points->Delete();
  triangles->Delete();
  this->Mesh->Delete();
  neighbors->Delete();
  cells->Delete();

  // If the best fitting option was ON, then the current transform
  // is the one that was computed internally. We must now destroy it.
  if( this->ProjectionPlaneMode == VTK_BEST_FITTING_PLANE )
    {
    if (this->Transform)
      {
      this->Transform->UnRegister(this);
      this->Transform = NULL;
      }
    }

  output->Squeeze();

  return 1;
}

// Methods used to recover edges. Uses lines and polygons to determine boundary
// and inside/outside.
//
// Only the topology of the Source is used during the constrain operation.
// The point ids in the Source topology are assumed to reference points
// in the input. So, when an input transform is used, only the input points
// are transformed.  We do not bother with transforming the Source points
// since they are never referenced.
int *vtkDelaunay2D::RecoverBoundary(vtkPolyData *source)
{
  vtkCellArray *lines=source->GetLines();
  vtkCellArray *polys=source->GetPolys();
  vtkIdType *pts = 0;
  vtkIdType npts = 0;
  vtkIdType i, p1, p2;
  int *triUse;

  // Recover the edges of the mesh
  for ( lines->InitTraversal(); lines->GetNextCell(npts,pts); )
    {
    for (i=0; i<(npts-1); i++)
      {
      p1 = pts[i];
      p2 = pts[i+1];
      if ( ! this->Mesh->IsEdge(p1,p2) )
        {
        this->RecoverEdge(p1, p2);
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
      if ( ! this->Mesh->IsEdge(p1,p2) )
        {
        this->RecoverEdge(p1, p2);
        }
      }
    }

  // Generate inside/outside marks on mesh
  int numTriangles = this->Mesh->GetNumberOfCells();
  triUse = new int[numTriangles];
  for (i=0; i<numTriangles; i++)
    {
    triUse[i] = 1;
    }

  // Use any polygons to mark inside and outside. (Note that if an edge was not
  // recovered, we're going to have a problem.) The first polygon is assumed to
  // define the outside of the polygon; additional polygons carve out inside
  // holes.
  this->FillPolygons(polys, triUse);

  return triUse;
}

// Method attempts to recover an edge by retriangulating mesh around the edge.
// What we do is identify a "submesh" of triangles that includes the edge to recover.
// Then we split the submesh in two with the recovered edge, and triangulate each of
// the two halves. If any part of this fails, we leave things alone.
int vtkDelaunay2D::RecoverEdge(vtkIdType p1, vtkIdType p2)
{
  vtkIdType cellId = 0;
  int i, j, k;
  double p1X[3], p2X[3], xyNormal[3], splitNormal[3], p21[3];
  double x1[3], x2[3], sepNormal[3], v21[3];
  int ncells, v1=0, v2=0, signX1=0, signX2, signP1, signP2;
  vtkIdType *pts, *leftTris, *rightTris, npts, numRightTris, numLeftTris;
  int success=0;

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
  this->GetPoint(p1,p1X); p1X[2] = 0.0; //split plane point
  this->GetPoint(p2,p2X); p2X[2] = 0.0; //split plane point
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
  this->Mesh->GetPointCells(p1, cells);
  ncells = cells->GetNumberOfIds();
  for (i=0; i < ncells; i++)
    {
    cellId = cells->GetId(i);
    this->Mesh->GetCellPoints(cellId, npts, pts);
    for (j=0; j<3; j++)
      {
      if ( pts[j] == p1 )
        {
        break;
        }
      }
    v1 = pts[(j+1)%3];
    v2 = pts[(j+2)%3];
    this->GetPoint(v1,x1); x1[2] = 0.0;
    this->GetPoint(v2,x2); x2[2] = 0.0;
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
    this->Mesh->GetCellEdgeNeighbors(cellId, v1, v2, neis);
    if ( neis->GetNumberOfIds() != 1 )
      {//Mesh is folded or degenerate
      goto FAILURE;
      }
    cellId = neis->GetId(0);
    tris->InsertNextId(cellId);
    this->Mesh->GetCellPoints(cellId, npts, pts);
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
          this->GetPoint(pts[j], x1); x1[2] = 0.0;
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
    this->Mesh->RemoveCellReference(cellId);
    for (k=0; k<3; k++)
      {//allocate new space for cell lists
      this->Mesh->ResizeCellList(leftTris[k],1);
      }
    this->Mesh->ReplaceLinkedCell(cellId, 3, leftTris);
    }

  rightTris = rightPtIds->GetPointer(0);
  for ( i=0; i<numRightTris; i++, j++, rightTris+=3)
    {
    cellId = tris->GetId(j);
    this->Mesh->RemoveCellReference(cellId);
    for (k=0; k<3; k++)
      {//allocate new space for cell lists
      this->Mesh->ResizeCellList(rightTris[k],1);
      }
    this->Mesh->ReplaceLinkedCell(cellId, 3, rightTris);
    }

  FAILURE:
    tris->Delete(); cells->Delete();
    leftPoly->Delete(); rightPoly->Delete(); neis->Delete();
    rightPtIds->Delete(); leftPtIds->Delete();
    rightTriPts->Delete(); leftTriPts->Delete();
    return success;
}

void vtkDelaunay2D::FillPolygons(vtkCellArray *polys, int *triUse)
{
  vtkIdType p1, p2, j, kk;
  int i, k;
  vtkIdType *pts = 0;
  vtkIdType *triPts;
  vtkIdType npts = 0;
  vtkIdType numPts;
  static double xyNormal[3]={0.0,0.0,1.0};
  double negDir[3], x21[3], x1[3], x2[3], x[3];
  vtkIdList *neis=vtkIdList::New();
  vtkIdType cellId, numNeis;
  vtkIdList *currentFront = vtkIdList::New(), *tmpFront;
  vtkIdList *nextFront = vtkIdList::New();
  vtkIdType numCellsInFront, neiId;
  vtkIdType numTriangles=this->Mesh->GetNumberOfCells();

  // Loop over edges of polygon, marking triangles on "outside" of polygon as outside.
  // Then perform a fill.
  for ( polys->InitTraversal(); polys->GetNextCell(npts,pts); )
    {
    currentFront->Reset();
    for (i=0; i<npts; i++)
      {
      p1 = pts[i];
      p2 = pts[(i+1)%npts];
      if ( ! this->Mesh->IsEdge(p1,p2) )
        {
        vtkWarningMacro(<<"Edge not recovered, polygon fill suspect");
        }
      else //Mark the "outside" triangles
        {
        neis->Reset();
        this->GetPoint(p1,x1);
        this->GetPoint(p2,x2);
        for (j=0; j<3; j++)
          {
          x21[j] = x2[j] - x1[j];
          }
        vtkMath::Cross (x21,xyNormal,negDir);
        this->Mesh->GetCellEdgeNeighbors(-1, p1, p2, neis); //get both triangles
        numNeis = neis->GetNumberOfIds();
        for (j=0; j<numNeis; j++)
          {//find the vertex not on the edge; evaluate it (and the cell) in/out
          cellId = neis->GetId(j);
          this->Mesh->GetCellPoints(cellId, numPts, triPts);
          for (k=0; k<3; k++)
            {
            if ( triPts[k] != p1 && triPts[k] != p2 )
              {
              break;
              }
            }
          this->GetPoint(triPts[k],x); x[2] = 0.0;
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

        this->Mesh->GetCellPoints(cellId, numPts, triPts);
        for (k=0; k<3; k++)
          {
          p1 = triPts[k];
          p2 = triPts[(k+1)%3];

          this->Mesh->GetCellEdgeNeighbors(cellId, p1, p2, neis);
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

//----------------------------------------------------------------------------
int vtkDelaunay2D::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkAbstractTransform * vtkDelaunay2D::ComputeBestFittingPlane(
  vtkPointSet *input)
{
  vtkIdType numPts=input->GetNumberOfPoints();
  double m[9], v[3], x[3];
  vtkIdType ptId;
  int i;
  double *c1, *c2, *c3, det;
  double normal[3];
  double origin[3];

  const double tolerance = 1.0e-03;

  //  This code was taken from the vtkTextureMapToPlane class
  //  and slightly modified.
  //
  for (i=0; i<3; i++)
    {
    normal[i] = 0.0;
    }

  //  Compute least squares approximation.
  //  Compute 3x3 least squares matrix
  v[0] = v[1] = v[2] = 0.0;
  for (i=0; i<9; i++)
    {
    m[i] = 0.0;
    }

  for (ptId=0; ptId < numPts; ptId++)
    {
    input->GetPoint(ptId, x);

    v[0] += x[0]*x[2];
    v[1] += x[1]*x[2];
    v[2] += x[2];

    m[0] += x[0]*x[0];
    m[1] += x[0]*x[1];
    m[2] += x[0];

    m[3] += x[0]*x[1];
    m[4] += x[1]*x[1];
    m[5] += x[1];

    m[6] += x[0];
    m[7] += x[1];
    }
  m[8] = numPts;

  origin[0] = m[2] / numPts;
  origin[1] = m[5] / numPts;
  origin[2] = v[2] / numPts;

  //  Solve linear system using Kramers rule
  //
  c1 = m; c2 = m+3; c3 = m+6;
  if ( (det = vtkMath::Determinant3x3 (c1,c2,c3)) > tolerance )
    {
    normal[0] =  vtkMath::Determinant3x3 (v,c2,c3) / det;
    normal[1] =  vtkMath::Determinant3x3 (c1,v,c3) / det;
    normal[2] = -1.0; // because of the formulation
    }

  vtkTransform * transform = vtkTransform::New();

  // Set the new Z axis as the normal to the best fitting
  // plane.
  double zaxis[3];
  zaxis[0] = 0;
  zaxis[1] = 0;
  zaxis[2] = 1;

  double rotationAxis[3];

  vtkMath::Normalize(normal);
  vtkMath::Cross(normal,zaxis,rotationAxis);
  vtkMath::Normalize(rotationAxis);

  const double rotationAngle =
    180.0*acos(vtkMath::Dot(zaxis,normal))/vtkMath::Pi();

  transform->PreMultiply();
  transform->Identity();

  transform->RotateWXYZ(rotationAngle,
    rotationAxis[0], rotationAxis[1], rotationAxis[2]);

  // Set the center of mass as the origin of coordinates
  transform->Translate( -origin[0], -origin[1], -origin[2] );

  return transform;
}

//----------------------------------------------------------------------------
void vtkDelaunay2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Alpha: " << this->Alpha << "\n";
  os << indent << "ProjectionPlaneMode: "
     << ((this->ProjectionPlaneMode == VTK_BEST_FITTING_PLANE)? "Best Fitting Plane" : "XY Plane") << "\n";
  os << indent << "Transform: " << (this->Transform ? "specified" : "none") << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "Offset: " << this->Offset << "\n";
  os << indent << "Bounding Triangulation: "
     << (this->BoundingTriangulation ? "On\n" : "Off\n");
}
