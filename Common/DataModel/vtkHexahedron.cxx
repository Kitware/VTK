/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHexahedron.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHexahedron.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkPoints.h"
#include "vtkQuad.h"
#include "vtkMarchingCubesTriangleCases.h"

vtkStandardNewMacro(vtkHexahedron);

static const double VTK_DIVERGED = 1.e6;
static const int VTK_HEX_MAX_ITERATION=10;
static const double VTK_HEX_CONVERGED=1.e-03;
static const double VTK_HEX_OUTSIDE_CELL_TOLERANCE=1.e-06;

//----------------------------------------------------------------------------
// Construct the hexahedron with eight points.
vtkHexahedron::vtkHexahedron()
{
  this->Points->SetNumberOfPoints(8);
  this->PointIds->SetNumberOfIds(8);

  for (int i = 0; i < 8; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
  }
  this->Line = vtkLine::New();
  this->Quad = vtkQuad::New();
}

//----------------------------------------------------------------------------
vtkHexahedron::~vtkHexahedron()
{
  this->Line->Delete();
  this->Quad->Delete();
}

//----------------------------------------------------------------------------
//  Method to calculate parametric coordinates in an eight noded
//  linear hexahedron element from global coordinates.
//
int vtkHexahedron::EvaluatePosition(double x[3], double* closestPoint,
                                   int& subId, double pcoords[3],
                                   double& dist2, double *weights)
{
  int iteration, converged;
  double  params[3];
  double  fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  double  d, pt[3];
  double derivs[24];

  //  set initial position for Newton's method
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2]=0.5;

  //  enter iteration loop
  for (iteration=converged=0;
       !converged && (iteration < VTK_HEX_MAX_ITERATION);  iteration++)
  {
    //  calculate element interpolation functions and derivatives
    this->InterpolationFunctions(pcoords, weights);
    this->InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    for (i=0; i<3; i++)
    {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
    }
    for (i=0; i<8; i++)
    {
      this->Points->GetPoint(i, pt);
      for (j=0; j<3; j++)
      {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i+8];
        tcol[j] += pt[j] * derivs[i+16];
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
      return -1;
    }

    pcoords[0] = params[0] - vtkMath::Determinant3x3 (fcol,scol,tcol) / d;
    pcoords[1] = params[1] - vtkMath::Determinant3x3 (rcol,fcol,tcol) / d;
    pcoords[2] = params[2] - vtkMath::Determinant3x3 (rcol,scol,fcol) / d;

    //  check for convergence
    if ( ((fabs(pcoords[0]-params[0])) < VTK_HEX_CONVERGED) &&
         ((fabs(pcoords[1]-params[1])) < VTK_HEX_CONVERGED) &&
         ((fabs(pcoords[2]-params[2])) < VTK_HEX_CONVERGED) )
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

  double lowerlimit = 0.0-VTK_HEX_OUTSIDE_CELL_TOLERANCE;
  double upperlimit = 1.0+VTK_HEX_OUTSIDE_CELL_TOLERANCE;
  if ( pcoords[0] >= lowerlimit && pcoords[0] <= upperlimit &&
       pcoords[1] >= lowerlimit && pcoords[1] <= upperlimit &&
       pcoords[2] >= lowerlimit && pcoords[2] <= upperlimit )
  {
    if (closestPoint)
    {
      closestPoint[0] = x[0]; closestPoint[1] = x[1]; closestPoint[2] = x[2];
      dist2 = 0.0; //inside hexahedron
    }
    return 1;
  }
  else
  {
    double pc[3], w[8];
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
// Compute iso-parametric interpolation functions
//
void vtkHexahedron::InterpolationFunctions(double pcoords[3], double sf[8])
{
  double rm, sm, tm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];
  tm = 1. - pcoords[2];

  sf[0] = rm*sm*tm;
  sf[1] = pcoords[0]*sm*tm;
  sf[2] = pcoords[0]*pcoords[1]*tm;
  sf[3] = rm*pcoords[1]*tm;
  sf[4] = rm*sm*pcoords[2];
  sf[5] = pcoords[0]*sm*pcoords[2];
  sf[6] = pcoords[0]*pcoords[1]*pcoords[2];
  sf[7] = rm*pcoords[1]*pcoords[2];
}

//----------------------------------------------------------------------------
void vtkHexahedron::InterpolationDerivs(double pcoords[3], double derivs[24])
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
  derivs[4] = -sm*pcoords[2];
  derivs[5] = sm*pcoords[2];
  derivs[6] = pcoords[1]*pcoords[2];
  derivs[7] = -pcoords[1]*pcoords[2];

  // s-derivatives
  derivs[8] = -rm*tm;
  derivs[9] = -pcoords[0]*tm;
  derivs[10] = pcoords[0]*tm;
  derivs[11] = rm*tm;
  derivs[12] = -rm*pcoords[2];
  derivs[13] = -pcoords[0]*pcoords[2];
  derivs[14] = pcoords[0]*pcoords[2];
  derivs[15] = rm*pcoords[2];

  // t-derivatives
  derivs[16] = -rm*sm;
  derivs[17] = -pcoords[0]*sm;
  derivs[18] = -pcoords[0]*pcoords[1];
  derivs[19] = -rm*pcoords[1];
  derivs[20] = rm*sm;
  derivs[21] = pcoords[0]*sm;
  derivs[22] = pcoords[0]*pcoords[1];
  derivs[23] = rm*pcoords[1];
}

//----------------------------------------------------------------------------
void vtkHexahedron::EvaluateLocation(int& vtkNotUsed(subId), double pcoords[3],
                                     double x[3], double *weights)
{
  int i, j;
  double pt[3];

  this->InterpolationFunctions(pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<8; i++)
  {
    this->Points->GetPoint(i, pt);
    for (j=0; j<3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//----------------------------------------------------------------------------
int vtkHexahedron::CellBoundary(int vtkNotUsed(subId), double pcoords[3],
                                vtkIdList *pts)
{
  double t1=pcoords[0]-pcoords[1];
  double t2=1.0-pcoords[0]-pcoords[1];
  double t3=pcoords[1]-pcoords[2];
  double t4=1.0-pcoords[1]-pcoords[2];
  double t5=pcoords[2]-pcoords[0];
  double t6=1.0-pcoords[2]-pcoords[0];

  pts->SetNumberOfIds(4);

  // compare against six planes in parametric space that divide element
  // into six pieces.
  if ( t3 >= 0.0 && t4 >= 0.0 && t5 < 0.0 && t6 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(2));
    pts->SetId(3,this->PointIds->GetId(3));
  }

  else if ( t1 >= 0.0 && t2 < 0.0 && t5 < 0.0 && t6 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(2));
    pts->SetId(2,this->PointIds->GetId(6));
    pts->SetId(3,this->PointIds->GetId(5));
  }

  else if ( t1 >= 0.0 && t2 >= 0.0 && t3 < 0.0 && t4 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(5));
    pts->SetId(3,this->PointIds->GetId(4));
  }

  else if ( t3 < 0.0 && t4 < 0.0 && t5 >= 0.0 && t6 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(4));
    pts->SetId(1,this->PointIds->GetId(5));
    pts->SetId(2,this->PointIds->GetId(6));
    pts->SetId(3,this->PointIds->GetId(7));
  }

  else if ( t1 < 0.0 && t2 >= 0.0 && t5 >= 0.0 && t6 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(4));
    pts->SetId(2,this->PointIds->GetId(7));
    pts->SetId(3,this->PointIds->GetId(3));
  }

  else // if ( t1 < 0.0 && t2 < 0.0 && t3 >= 0.0 && t6 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(3));
    pts->SetId(2,this->PointIds->GetId(7));
    pts->SetId(3,this->PointIds->GetId(6));
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
static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                            {4,5}, {5,6}, {7,6}, {4,7},
                            {0,4}, {1,5}, {3,7}, {2,6}};
static int faces[6][5] = { {0,4,7,3,-1}, {1,2,6,5,-1},
                           {0,1,5,4,-1}, {3,7,6,2,-1},
                           {0,3,2,1,-1}, {4,5,6,7,-1} };

// Marching cubes case table
//
#include "vtkMarchingCubesTriangleCases.h"

void vtkHexahedron::Contour(double value, vtkDataArray *cellScalars,
                            vtkIncrementalPointLocator *locator,
                            vtkCellArray *verts,
                            vtkCellArray *lines,
                            vtkCellArray *polys,
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId,
                            vtkCellData *outCd)
{
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  vtkMarchingCubesTriangleCases *triCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  int v1, v2, newCellId;
  vtkIdType pts[3];
  double t, x1[3], x2[3], x[3], deltaScalar;
  vtkIdType offset = verts->GetNumberOfCells() + lines->GetNumberOfCells();

  // Build the case table
  for ( i=0, index = 0; i < 8; i++)
  {
    if (cellScalars->GetComponent(i,0) >= value)
    {
      index |= CASE_MASK[i];
    }
  }

  triCase = vtkMarchingCubesTriangleCases::GetCases() + index;
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
int *vtkHexahedron::GetEdgeArray(int edgeId)
{
  return edges[edgeId];
}

//----------------------------------------------------------------------------
vtkCell *vtkHexahedron::GetEdge(int edgeId)
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
int *vtkHexahedron::GetFaceArray(int faceId)
{
  return faces[faceId];
}

//----------------------------------------------------------------------------
vtkCell *vtkHexahedron::GetFace(int faceId)
{
  int *verts, i;

  verts = faces[faceId];

  for (i=0; i<4; i++)
  {
    this->Quad->PointIds->SetId(i,this->PointIds->GetId(verts[i]));
    this->Quad->Points->SetPoint(i,this->Points->GetPoint(verts[i]));
  }

  return this->Quad;
}

//----------------------------------------------------------------------------
//
// Intersect hexa faces against line. Each hexa face is a quadrilateral.
//
int vtkHexahedron::IntersectWithLine(double p1[3], double p2[3], double tol,
                                    double &t, double x[3], double pcoords[3],
                                    int& subId)
{
  int intersection=0;
  double pt1[3], pt2[3], pt3[3], pt4[3];
  double tTemp;
  double pc[3], xTemp[3];
  int faceNum;

  t = VTK_DOUBLE_MAX;
  for (faceNum=0; faceNum<6; faceNum++)
  {
    this->Points->GetPoint(faces[faceNum][0], pt1);
    this->Points->GetPoint(faces[faceNum][1], pt2);
    this->Points->GetPoint(faces[faceNum][2], pt3);
    this->Points->GetPoint(faces[faceNum][3], pt4);

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
        switch (faceNum)
        {
          case 0:
            pcoords[0] = 0.0; pcoords[1] = pc[0]; pcoords[2] = 0.0;
            break;

          case 1:
            pcoords[0] = 1.0; pcoords[1] = pc[0]; pcoords[2] = 0.0;
            break;

          case 2:
            pcoords[0] = pc[0]; pcoords[1] = 0.0; pcoords[2] = pc[1];
            break;

          case 3:
            pcoords[0] = pc[0]; pcoords[1] = 1.0; pcoords[2] = pc[1];
            break;

          case 4:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 0.0;
            break;

          case 5:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 1.0;
            break;
        }
      }
    }
  }
  return intersection;
}

//----------------------------------------------------------------------------
int vtkHexahedron::Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts)
{
  int p[4], i;

  ptIds->Reset();
  pts->Reset();

  // Create five tetrahedron. Triangulation varies depending upon index. This
  // is necessary to insure compatible voxel triangulations.
  if ( (index % 2) )
  {
    p[0] = 0; p[1] = 1; p[2] = 3; p[3] = 4;
    for ( i=0; i < 4; i++ )
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 1; p[1] = 4; p[2] = 5; p[3] = 6;
    for ( i=0; i < 4; i++ )
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 1; p[1] = 4; p[2] = 6; p[3] = 3;
    for ( i=0; i < 4; i++ )
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 1; p[1] = 3; p[2] = 6; p[3] = 2;
    for ( i=0; i < 4; i++ )
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 3; p[1] = 6; p[2] = 7; p[3] = 4;
    for ( i=0; i < 4; i++ )
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }
  }
  else
  {
    p[0] = 2; p[1] = 1; p[2] = 5; p[3] = 0;
    for ( i=0; i < 4; i++ )
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 0; p[1] = 2; p[2] = 3; p[3] = 7;
    for ( i=0; i < 4; i++ )
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 2; p[1] = 5; p[2] = 6; p[3] = 7;
    for ( i=0; i < 4; i++ )
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 0; p[1] = 7; p[2] = 4; p[3] = 5;
    for ( i=0; i < 4; i++ )
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }

    p[0] = 0; p[1] = 2; p[2] = 7; p[3] = 5;
    for ( i=0; i < 4; i++ )
    {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
// Compute derivatives in x-y-z directions. Use chain rule in combination
// with interpolation function derivatives.
//
void vtkHexahedron::Derivatives(int vtkNotUsed(subId), double pcoords[3],
                                double *values, int dim, double *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[24], sum[3];
  int i, j, k;

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (k=0; k < dim; k++) //loop over values per vertex
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for ( i=0; i < 8; i++) //loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim*i + k];
      sum[1] += functionDerivs[8 + i] * values[dim*i + k];
      sum[2] += functionDerivs[16 + i] * values[dim*i + k];
    }
    for (j=0; j < 3; j++) //loop over derivative directions
    {
      derivs[3*k + j] = sum[0]*jI[j][0] + sum[1]*jI[j][1] + sum[2]*jI[j][2];
    }
  }
}

//----------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkHexahedron::JacobianInverse(double pcoords[3], double **inverse,
                                    double derivs[24])
{
  int i, j;
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  this->InterpolationDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0; m[1] = m1; m[2] = m2;
  for (i=0; i < 3; i++) //initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for ( j=0; j < 8; j++ )
  {
    this->Points->GetPoint(j, x);
    for ( i=0; i < 3; i++ )
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[8 + j];
      m2[i] += x[i] * derivs[16 + j];
    }
  }

  // now find the inverse
  if ( vtkMath::InvertMatrix(m,inverse,3) == 0 )
  {
    vtkErrorMacro(<<"Jacobian inverse not found");
    return;
  }
}

//----------------------------------------------------------------------------
void vtkHexahedron::GetEdgePoints(int edgeId, int* &pts)
{
  pts = this->GetEdgeArray(edgeId);
}

//----------------------------------------------------------------------------
void vtkHexahedron::GetFacePoints(int faceId, int* &pts)
{
  pts = this->GetFaceArray(faceId);
}

//----------------------------------------------------------------------------
static double vtkHexahedronCellPCoords[24] = {0.0,0.0,0.0, 1.0,0.0,0.0,
                                             1.0,1.0,0.0, 0.0,1.0,0.0,
                                             0.0,0.0,1.0, 1.0,0.0,1.0,
                                             1.0,1.0,1.0, 0.0,1.0,1.0};

double *vtkHexahedron::GetParametricCoords()
{
  return vtkHexahedronCellPCoords;
}

//----------------------------------------------------------------------------
void vtkHexahedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Quad:\n";
  this->Quad->PrintSelf(os,indent.GetNextIndent());
}

