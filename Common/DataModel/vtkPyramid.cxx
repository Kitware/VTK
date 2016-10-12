/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPyramid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPyramid.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkPyramid);

namespace
{
  static const double VTK_DIVERGED = 1.e6;
  static const int VTK_MAX_ITERATION=10;
  static const double VTK_CONVERGED=1.e-03;
}

//----------------------------------------------------------------------------
//
// Construct the pyramid with five points.
//
vtkPyramid::vtkPyramid()
{
  this->Points->SetNumberOfPoints(5);
  this->PointIds->SetNumberOfIds(5);
  for (int i = 0; i < 5; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
  }
  this->Line = vtkLine::New();
  this->Triangle = vtkTriangle::New();
  this->Quad = vtkQuad::New();
}

//----------------------------------------------------------------------------
vtkPyramid::~vtkPyramid()
{
  this->Line->Delete();
  this->Triangle->Delete();
  this->Quad->Delete();
}

//----------------------------------------------------------------------------
int vtkPyramid::EvaluatePosition(double x[3], double closestPoint[3],
                                 int& subId, double pcoords[3],
                                 double& dist2, double *weights)
{
  int i, j;
  subId = 0;
  // There are problems searching for the apex point so we check if
  // we are there first before doing the full parametric inversion.
  vtkPoints* points = this->GetPoints();
  double apexPoint[3];
  points->GetPoint(4, apexPoint);
  dist2 = vtkMath::Distance2BetweenPoints(apexPoint, x);
  double baseMidpoint[3];
  points->GetPoint(0, baseMidpoint);
  for(i=1;i<4;i++)
  {
    double tmp[3];
    points->GetPoint(i, tmp);
    for(j=0;j<3;j++)
    {
      baseMidpoint[j] += tmp[j];
    }
  }
  for(i=0;i<3;i++)
  {
    baseMidpoint[i] /= 4.;
  }

  double length2 = vtkMath::Distance2BetweenPoints(apexPoint, baseMidpoint);
  // we use .001 as the relative tolerance here since that is the same
  // that is used for the interior cell check below but we need to
  // square it here because we're looking at dist2^2.
  if(dist2 == 0. || ( length2 != 0. && dist2/length2 < 1.e-6) )
  {
    pcoords[0] = pcoords[1] = 0;
    pcoords[2] = 1;
    this->InterpolationFunctions(pcoords, weights);
    if(closestPoint)
    {
      memcpy(closestPoint, x, 3*sizeof(double));
      dist2 = 0.;
    }
    return 1;
  }

  int iteration, converged;
  double  params[3];
  double  fcol[3], rcol[3], scol[3], tcol[3];
  double  d, pt[3];
  double derivs[15];

  //  set initial position for Newton's method
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  params[0] = params[1] = params[2] = 0.3333333;

  //  enter iteration loop
  for (iteration=converged=0; !converged && (iteration < VTK_MAX_ITERATION);
  iteration++)
  {
    //  calculate element interpolation functions and derivatives
    this->InterpolationFunctions(pcoords, weights);
    this->InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    for (i=0; i<3; i++)
    {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
    }
    for (i=0; i<5; i++)
    {
      this->Points->GetPoint(i, pt);
      for (j=0; j<3; j++)
      {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i+5];
        tcol[j] += pt[j] * derivs[i+10];
      }
    }

    for (i=0; i<3; i++)
    {
      fcol[i] -= x[i];
    }

    //  compute determinants and generate improvements
    d=vtkMath::Determinant3x3(rcol,scol,tcol);
    if ( fabs(d) < 1.e-20)
    {
      vtkDebugMacro (<<"Determinant incorrect, iteration " << iteration);
      return -1;
    }

    pcoords[0] = params[0] - vtkMath::Determinant3x3 (fcol,scol,tcol) / d;
    pcoords[1] = params[1] - vtkMath::Determinant3x3 (rcol,fcol,tcol) / d;
    pcoords[2] = params[2] - vtkMath::Determinant3x3 (rcol,scol,fcol) / d;

    //  check for convergence
    if ( ((fabs(pcoords[0]-params[0])) < VTK_CONVERGED) &&
         ((fabs(pcoords[1]-params[1])) < VTK_CONVERGED) &&
         ((fabs(pcoords[2]-params[2])) < VTK_CONVERGED) )
    {
      converged = 1;
    }
    // Test for bad divergence (S.Hirschberg 11.12.2001)
    else if ((fabs(pcoords[0]) > VTK_DIVERGED) ||
             (fabs(pcoords[1]) > VTK_DIVERGED) ||
             (fabs(pcoords[2]) > VTK_DIVERGED))
    {
      return -1;
    }
    //  if not converged, repeat
    else
    {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
    }
  }

  //  if not converged, set the parametric coordinates to arbitrary values
  //  outside of element
  if ( !converged )
  {
    return -1;
  }

  this->InterpolationFunctions(pcoords, weights);

  if ( pcoords[0] >= -0.001 && pcoords[0] <= 1.001 &&
  pcoords[1] >= -0.001 && pcoords[1] <= 1.001 &&
  pcoords[2] >= -0.001 && pcoords[2] <= 1.001 )
  {
    if (closestPoint)
    {
      closestPoint[0] = x[0]; closestPoint[1] = x[1]; closestPoint[2] = x[2];
      dist2 = 0.0; //inside pyramid
    }
    return 1;
  }
  else
  {
    double pc[3], w[5];
    if (closestPoint)
    {
      for (i=0; i<3; i++) //only approximate, not really true for warped hexa
      {
        if (pcoords[i] < 0.0)
        {
          pc[i] = 0.0;
        }
        else if (pcoords[i] > 1.0)
        {
          pc[i] = 1.0;
        }
        else
        {
          pc[i] = pcoords[i];
        }
      }
      this->EvaluateLocation(subId, pc, closestPoint,
                             static_cast<double *>(w));
      dist2 = vtkMath::Distance2BetweenPoints(closestPoint,x);
    }
    return 0;
  }
}

//----------------------------------------------------------------------------
void vtkPyramid::EvaluateLocation(int& vtkNotUsed(subId), double pcoords[3],
                                double x[3], double *weights)
{
  int i, j;
  double pt[3];

  this->InterpolationFunctions(pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<5; i++)
  {
    this->Points->GetPoint(i, pt);
    for (j=0; j<3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//----------------------------------------------------------------------------
// Returns the closest face to the point specified. Closeness is measured
// parametrically.
int vtkPyramid::CellBoundary(int vtkNotUsed(subId), double pcoords[3],
                           vtkIdList *pts)
{
  int i;

  // define 6 planes that separate regions
  static double normals[6][3] = {
    {0.0,-0.5547002,0.8320503}, {0.5547002,0.0,0.8320503}, {0.0,0.5547002,0.8320503},
    {-0.5547002,0.0,0.8320503}, {0.70710670,-0.70710670,0.0}, {0.70710670,0.70710670,0.0} };
  static double point[3] = {0.5,0.5,0.3333333};
  double vals[6];

  // evaluate 6 plane equations
  for (i=0; i<6; i++)
  {
    vals[i] = normals[i][0]*(pcoords[0]-point[0]) +
      normals[i][1]*(pcoords[1]-point[1]) + normals[i][2]*(pcoords[2]-point[2]);
  }

  // compare against six planes in parametric space that divide element
  // into five pieces (each corresponding to a face).
  if ( vals[4] >= 0.0 && vals[5] <= 0.0 && vals[0] >= 0.0 )
  {
    pts->SetNumberOfIds(3); //triangle face
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(4));
  }

  else if ( vals[4] >= 0.0 && vals[5] >= 0.0 && vals[1] >= 0.0 )
  {
    pts->SetNumberOfIds(3); //triangle face
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(2));
    pts->SetId(2,this->PointIds->GetId(4));
  }

  else if ( vals[4] <= 0.0 && vals[5] >= 0.0 && vals[2] >= 0.0 )
  {
    pts->SetNumberOfIds(3); //triangle face
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(3));
    pts->SetId(2,this->PointIds->GetId(4));
  }

  else if ( vals[4] <= 0.0 && vals[5] <= 0.0 && vals[3] >= 0.0 )
  {
    pts->SetNumberOfIds(3); //triangle face
    pts->SetId(0,this->PointIds->GetId(3));
    pts->SetId(1,this->PointIds->GetId(0));
    pts->SetId(2,this->PointIds->GetId(4));
  }

  else
  {
    pts->SetNumberOfIds(4); //quad face
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(2));
    pts->SetId(3,this->PointIds->GetId(3));
  }

  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
       pcoords[1] < 0.0 || pcoords[1] > 1.0 ||
       pcoords[2] < 0.0 || pcoords[2] > 1.0 )
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
// Marching pyramids (contouring)
//
static int edges[8][2] = { {0,1}, {1,2}, {2,3},
                           {3,0}, {0,4}, {1,4},
                           {2,4}, {3,4} };
static int faces[5][5] = { {0,3,2,1,-1}, {0,1,4,-1,-1},
                           {1,2,4,-1,-1}, {2,3,4,-1,-1}, {3,0,4,-1,-1} };

typedef int EDGE_LIST;
typedef struct {
       EDGE_LIST edges[13];
} TRIANGLE_CASES;

static TRIANGLE_CASES triCases[] = {
  {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //0
  {{ 3,  4,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //1
  {{ 5,  1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //2
  {{ 5,  1,  4,  1,  3,  4, -1, -1, -1, -1, -1, -1, -1}}, //3
  {{ 6,  2,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //4
  {{ 3,  4,  0,  6,  2,  1, -1, -1, -1, -1, -1, -1, -1}}, //5
  {{ 5,  2,  0,  6,  2,  5, -1, -1, -1, -1, -1, -1, -1}}, //6
  {{ 2,  3,  4,  2,  4,  6,  4,  5,  6, -1, -1, -1, -1}}, //7
  {{ 2,  7,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //8
  {{ 2,  7,  4,  4,  0,  2, -1, -1, -1, -1, -1, -1, -1}}, //9
  {{ 5,  1,  0,  2,  7,  3, -1, -1, -1, -1, -1, -1, -1}}, //10
  {{ 5,  7,  4,  1,  7,  5,  2,  7,  1, -1, -1, -1, -1}}, //11
  {{ 6,  3,  1,  7,  3,  6, -1, -1, -1, -1, -1, -1, -1}}, //12
  {{ 4,  6,  7,  0,  6,  4,  1,  6,  0, -1, -1, -1, -1}}, //13
  {{ 7,  5,  6,  3,  5,  7,  0,  5,  3, -1, -1, -1, -1}}, //14
  {{ 7,  4,  5,  7,  5,  6, -1, -1, -1, -1, -1, -1, -1}}, //15
  {{ 7,  5,  4,  7,  6,  5, -1, -1, -1, -1, -1, -1, -1}}, //16
  {{ 5,  0,  3,  6,  5,  3,  7,  6,  3, -1, -1, -1, -1}}, //17
  {{ 1,  0,  4,  7,  1,  4,  6,  1,  7, -1, -1, -1, -1}}, //18
  {{ 6,  1,  3,  7,  6,  3, -1, -1, -1, -1, -1, -1, -1}}, //19
  {{ 7,  5,  4,  7,  1,  5,  7,  2,  1, -1, -1, -1, -1}}, //20
  {{ 3,  7,  0,  7,  5,  0,  7,  2,  5,  2,  1,  5, -1}}, //21
  {{ 4,  2,  0,  7,  2,  4, -1, -1, -1, -1, -1, -1, -1}}, //22
  {{ 7,  2,  3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //23
  {{ 2,  4,  3,  5,  4,  2,  6,  5,  2, -1, -1, -1, -1}}, //24
  {{ 2,  5,  0,  2,  6,  5, -1, -1, -1, -1, -1, -1, -1}}, //25
  {{ 6,  1,  0,  4,  6,  0,  3,  6,  4,  3,  2,  6, -1}}, //26
  {{ 2,  6,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //27
  {{ 1,  4,  3,  1,  5,  4, -1, -1, -1, -1, -1, -1, -1}}, //28
  {{ 1,  5,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //29
  {{ 4,  3,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}, //30
  {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}}  //31
};

//----------------------------------------------------------------------------
void vtkPyramid::Contour(double value, vtkDataArray *cellScalars,
                         vtkIncrementalPointLocator *locator,
                         vtkCellArray *verts,
                         vtkCellArray *lines,
                         vtkCellArray *polys,
                         vtkPointData *inPd, vtkPointData *outPd,
                         vtkCellData *inCd, vtkIdType cellId,
                         vtkCellData *outCd)
{
  static int CASE_MASK[5] = {1,2,4,8,16};
  TRIANGLE_CASES *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert, v1, v2, newCellId;
  vtkIdType pts[3];
  double t, x1[3], x2[3], x[3], deltaScalar;
  vtkIdType offset = verts->GetNumberOfCells() + lines->GetNumberOfCells();

  // Build the case table
  for ( i=0, index = 0; i < 5; i++)
  {
    if (cellScalars->GetComponent(i,0) >= value)
    {
      index |= CASE_MASK[i];
    }
  }

  triCase = triCases + index;
  edge = triCase->edges;

  for ( ; edge[0] > -1; edge += 3 )
  {
    for (i=0; i<3; i++) // insert triangle
    {
      vert = edges[edge[i]];

      // calculate a preferred interpolation direction
      deltaScalar = (cellScalars->GetComponent(vert[1],0)
                     - cellScalars->GetComponent(vert[0],0));
      if (deltaScalar > 0)
      {
        v1 = vert[0]; v2 = vert[1];
      }
      else
      {
        v1 = vert[1]; v2 = vert[0];
        deltaScalar = -deltaScalar;
      }

      // linear interpolation
      t = ( deltaScalar == 0.0 ? 0.0 :
            (value - cellScalars->GetComponent(v1,0)) / deltaScalar );

      this->Points->GetPoint(v1, x1);
      this->Points->GetPoint(v2, x2);

      for (j=0; j<3; j++)
      {
        x[j] = x1[j] + t * (x2[j] - x1[j]);
      }
      if ( locator->InsertUniquePoint(x, pts[i]) )
      {
        if ( outPd )
        {
          vtkIdType p1 = this->PointIds->GetId(v1);
          vtkIdType p2 = this->PointIds->GetId(v2);
          outPd->InterpolateEdge(inPd,pts[i],p1,p2,t);
        }
      }
    }

    // check for degenerate triangle
    if ( pts[0] != pts[1] && pts[0] != pts[2] && pts[1] != pts[2] )
    {
      newCellId = offset + polys->InsertNextCell(3,pts);
      outCd->CopyData(inCd,cellId,newCellId);
    }
  }
}

//----------------------------------------------------------------------------
int *vtkPyramid::GetEdgeArray(int edgeId)
{
  return edges[edgeId];
}

//----------------------------------------------------------------------------
vtkCell *vtkPyramid::GetEdge(int edgeId)
{
  int *verts;

  verts = edges[edgeId];

  // load point id's
  this->Line->PointIds->SetId(0,this->PointIds->GetId(verts[0]));
  this->Line->PointIds->SetId(1,this->PointIds->GetId(verts[1]));

  // load coordinates
  this->Line->Points->SetPoint(0,this->Points->GetPoint(verts[0]));
  this->Line->Points->SetPoint(1,this->Points->GetPoint(verts[1]));

  return this->Line;
}

//----------------------------------------------------------------------------
int *vtkPyramid::GetFaceArray(int faceId)
{
  return faces[faceId];
}

//----------------------------------------------------------------------------
vtkCell *vtkPyramid::GetFace(int faceId)
{
  int *verts;

  verts = faces[faceId];

  if ( verts[3] != -1 ) // quad cell
  {
    // load point id's
    this->Quad->PointIds->SetId(0,this->PointIds->GetId(verts[0]));
    this->Quad->PointIds->SetId(1,this->PointIds->GetId(verts[1]));
    this->Quad->PointIds->SetId(2,this->PointIds->GetId(verts[2]));
    this->Quad->PointIds->SetId(3,this->PointIds->GetId(verts[3]));

    // load coordinates
    this->Quad->Points->SetPoint(0,this->Points->GetPoint(verts[0]));
    this->Quad->Points->SetPoint(1,this->Points->GetPoint(verts[1]));
    this->Quad->Points->SetPoint(2,this->Points->GetPoint(verts[2]));
    this->Quad->Points->SetPoint(3,this->Points->GetPoint(verts[3]));

    return this->Quad;
  }
  else
  {
    // load point id's
    this->Triangle->PointIds->SetId(0,this->PointIds->GetId(verts[0]));
    this->Triangle->PointIds->SetId(1,this->PointIds->GetId(verts[1]));
    this->Triangle->PointIds->SetId(2,this->PointIds->GetId(verts[2]));

    // load coordinates
    this->Triangle->Points->SetPoint(0,this->Points->GetPoint(verts[0]));
    this->Triangle->Points->SetPoint(1,this->Points->GetPoint(verts[1]));
    this->Triangle->Points->SetPoint(2,this->Points->GetPoint(verts[2]));

    return this->Triangle;
  }
}

//----------------------------------------------------------------------------
// Intersect faces against line.
//
int vtkPyramid::IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                               double x[3], double pcoords[3], int& subId)
{
  int intersection=0;
  double pt1[3], pt2[3], pt3[3], pt4[3];
  double tTemp;
  double pc[3], xTemp[3], dist2, weights[5];

  int faceNum;

  t = VTK_DOUBLE_MAX;

  //first intersect the triangle faces
  for (faceNum=1; faceNum<5; faceNum++)
  {
    this->Points->GetPoint(faces[faceNum][0], pt1);
    this->Points->GetPoint(faces[faceNum][1], pt2);
    this->Points->GetPoint(faces[faceNum][2], pt3);

    this->Triangle->Points->SetPoint(0,pt1);
    this->Triangle->Points->SetPoint(1,pt2);
    this->Triangle->Points->SetPoint(2,pt3);

    if ( this->Triangle->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId) )
    {
      intersection = 1;
      if ( tTemp < t )
      {
        t = tTemp;
        x[0] = xTemp[0]; x[1] = xTemp[1]; x[2] = xTemp[2];
        this->EvaluatePosition(x, xTemp, subId, pcoords, dist2, weights);
      }
    }
  }

  //now intersect the quad face
  this->Points->GetPoint(faces[0][0], pt1);
  this->Points->GetPoint(faces[0][1], pt2);
  this->Points->GetPoint(faces[0][2], pt3);
  this->Points->GetPoint(faces[0][3], pt4);

  this->Quad->Points->SetPoint(0,pt1);
  this->Quad->Points->SetPoint(1,pt2);
  this->Quad->Points->SetPoint(2,pt3);
  this->Quad->Points->SetPoint(3,pt4);

  if ( this->Quad->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId) )
  {
    intersection = 1;
    if ( tTemp < t )
    {
      t = tTemp;
      x[0] = xTemp[0]; x[1] = xTemp[1]; x[2] = xTemp[2];
      pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 0.0;
    }
  }

  return intersection;
}

//----------------------------------------------------------------------------
int vtkPyramid::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, vtkPoints *pts)
{
  int p[4], i;
  ptIds->Reset();
  pts->Reset();

  // The base of the pyramid must be split into two triangles.  There are two
  // ways to do this (across either diagonal).  Pick the shorter diagonal.
  double base_points[4][3];
  for (i = 0; i < 4; i++)
  {
    this->Points->GetPoint(i, base_points[i]);
  }
  double diagonal1, diagonal2;
  diagonal1 = vtkMath::Distance2BetweenPoints(base_points[0], base_points[2]);
  diagonal2 = vtkMath::Distance2BetweenPoints(base_points[1], base_points[3]);

  if (diagonal1 < diagonal2)
  {
    for (i=0; i < 4; i++)
    {
      p[0] = 0; p[1] = 1; p[2] = 2; p[3] = 4;
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }
    for (i=0; i < 4; i++)
    {
      p[0] = 0; p[1] = 2; p[2] = 3; p[3] = 4;
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }
  }
  else
  {
    for (i=0; i < 4; i++)
    {
      p[0] = 0; p[1] = 1; p[2] = 3; p[3] = 4;
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }
    for (i=0; i < 4; i++)
    {
      p[0] = 1; p[1] = 2; p[2] = 3; p[3] = 4;
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }
  }

  return !(diagonal1 == diagonal2);
}

//----------------------------------------------------------------------------
void vtkPyramid::Derivatives(int subId, double pcoords[3],
                             double *values, int dim, double *derivs)
{
  if(pcoords[2] > .999)
  {
    // If we are at the apex of the pyramid we need to do something special.
    // As we approach the apex, the derivatives of the parametric shape
    // functions in x and y go to 0 while the inverse of the Jacobian
    // also goes to 0.  This results in 0/0 but using l'Hopital's rule
    // we could actually compute the value of the limit, if we had a
    // functional expression to compute the gradient.  We're on a computer
    // so we don't but we can cheat and do a linear extrapolation of the
    // derivatives which really ends up as the same thing.
    double pcoords1[3] = {.5, .5, 2.*.998-pcoords[2]};
    std::vector<double> derivs1(3*dim);
    this->Derivatives(subId, pcoords1, values, dim, &(derivs1[0]));
    double pcoords2[3] = {.5, .5, .998};
    std::vector<double> derivs2(3*dim);
    this->Derivatives(subId, pcoords2, values, dim, &(derivs2[0]));
    for(int i=0;i<dim*3;i++)
    {
      derivs[i] = 2.*derivs2[i] - derivs1[i];
    }
    return;
  }

  double functionDerivs[15], sum[3], value;
  int i, j, k;
  double *jI[3], j0[3], j1[3], j2[3];
  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (k=0; k < dim; k++) //loop over values per vertex
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for ( i=0; i < 5; i++) //loop over interp. function derivatives
    {
      value = values[dim*i + k];
      sum[0] += functionDerivs[i] * value;
      sum[1] += functionDerivs[5 + i] * value;
      sum[2] += functionDerivs[10 + i] * value;
    }

    for (j=0; j < 3; j++) //loop over derivative directions
    {
      derivs[3*k + j] = sum[0]*jI[j][0] + sum[1]*jI[j][1] + sum[2]*jI[j][2];
    }
  }
}

//----------------------------------------------------------------------------
// Compute iso-parametric interpolation functions for pyramid
//
void vtkPyramid::InterpolationFunctions(double pcoords[3], double sf[5])
{
  double rm, sm, tm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];
  tm = 1. - pcoords[2];

  sf[0] = rm*sm*tm;
  sf[1] = pcoords[0]*sm*tm;
  sf[2] = pcoords[0]*pcoords[1]*tm;
  sf[3] = rm*pcoords[1]*tm;
  sf[4] = pcoords[2];
}

//----------------------------------------------------------------------------
void vtkPyramid::InterpolationDerivs(double pcoords[3], double derivs[15])
{
  double rm, sm, tm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];
  tm = 1. - pcoords[2];

  // r-derivatives
  derivs[0] = -sm*tm;
  derivs[1] = sm*tm;
  derivs[2] = pcoords[1]*tm;
  derivs[3] = -pcoords[1]*tm;
  derivs[4] = 0.0;

  // s-derivatives
  derivs[5] = -rm*tm;
  derivs[6] = -pcoords[0]*tm;
  derivs[7] = pcoords[0]*tm;
  derivs[8] = rm*tm;
  derivs[9] = 0.0;

  // t-derivatives
  derivs[10] = -rm*sm;
  derivs[11] = -pcoords[0]*sm;
  derivs[12] = -pcoords[0]*pcoords[1];
  derivs[13] = -rm*pcoords[1];
  derivs[14] = 1.0;
}

//----------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives. Returns 0 if no inverse exists.
// Note for pyramid: the inverse Jacobian is undefined at the apex.
int vtkPyramid::JacobianInverse(double pcoords[3], double **inverse, double derivs[15])
{
  int i, j;
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  this->InterpolationDerivs(pcoords,derivs);

  // create Jacobian matrix
  m[0] = m0; m[1] = m1; m[2] = m2;
  for (i=0; i < 3; i++) //initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for ( j=0; j < 5; j++ )
  {
    this->Points->GetPoint(j, x);
    for ( i=0; i < 3; i++ )
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[5 + j];
      m2[i] += x[i] * derivs[10 + j];
    }
  }

  // now find the inverse
  if ( vtkMath::InvertMatrix(m,inverse,3) == 0 )
  {
#define VTK_MAX_WARNS 3
    static int numWarns=0;
    if ( numWarns++ < VTK_MAX_WARNS )
    {
      vtkErrorMacro(<<"Jacobian inverse not found");
      vtkErrorMacro(<<"Matrix:" << m[0][0] << " " << m[0][1] << " " << m[0][2]
      << m[1][0] << " " << m[1][1] << " " << m[1][2]
      << m[2][0] << " " << m[2][1] << " " << m[2][2] );
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPyramid::GetEdgePoints(int edgeId, int* &pts)
{
  pts = this->GetEdgeArray(edgeId);
}

//----------------------------------------------------------------------------
void vtkPyramid::GetFacePoints(int faceId, int* &pts)
{
  pts = this->GetFaceArray(faceId);
}

static double vtkPyramidCellPCoords[15] = {0.0,0.0,0.0, 1.0,0.0,0.0,
                                           1.0,1.0,0.0, 0.0,1.0,0.0,
                                           0.0,0.0,1.0};

//----------------------------------------------------------------------------
double *vtkPyramid::GetParametricCoords()
{
  return vtkPyramidCellPCoords;
}

//----------------------------------------------------------------------------
void vtkPyramid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Triangle:\n";
  this->Triangle->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Quad:\n";
  this->Quad->PrintSelf(os,indent.GetNextIndent());
}
