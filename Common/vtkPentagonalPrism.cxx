/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPentagonalPrism.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Thanks to Philippe Guerville who developed this class.
// Thanks to Charles Pignerol (CEA-DAM, France) who ported this class under
// VTK 4.
// Thanks to Jean Favre (CSCS, Switzerland) who contributed to integrate this
// class in VTK.
// Please address all comments to Jean Favre (jfavre at cscs.ch).

#include "vtkPentagonalPrism.h"

#include "vtkWedge.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkLine.h"
#include "vtkQuad.h"
#include "vtkPolygon.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkPoints.h"
#include "vtkPointData.h"

vtkCxxRevisionMacro(vtkPentagonalPrism, "1.6");
vtkStandardNewMacro(vtkPentagonalPrism);

static const double VTK_DIVERGED = 1.e6;

//----------------------------------------------------------------------------
// Construct the prism with ten points.
vtkPentagonalPrism::vtkPentagonalPrism()
{
  int i;
  this->DebugOff();
  this->Points->SetNumberOfPoints(10+2);
  this->PointIds->SetNumberOfIds(10+2);

// allocate enough room for the extra 2 points we insert on the pentagons
  for (i = 0; i < 10+2; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }
  this->Points->SetNumberOfPoints(10);
  this->PointIds->SetNumberOfIds(10);

  this->Line = vtkLine::New();
  this->Quad = vtkQuad::New();
  this->Wedge = vtkWedge::New();
  this->Polygon = vtkPolygon::New();
  this->Polygon->PointIds->SetNumberOfIds(5);
  this->Polygon->Points->SetNumberOfPoints(5);

  this->PointData = vtkPointData::New();
  this->CellData = vtkCellData::New();
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(6);  //num of vertices
}

//----------------------------------------------------------------------------
vtkPentagonalPrism::~vtkPentagonalPrism()
{
  this->Line->Delete();
  this->Quad->Delete();
  this->Polygon->Delete();
  this->Wedge->Delete();

  this->PointData->Delete();
  this->CellData->Delete();
  this->Scalars->Delete();
}

//
//  Method to calculate parametric coordinates in an ten noded
//  linear prism element from global coordinates.
//
static const int VTK_PENTA_MAX_ITERATION=10;
static const double VTK_PENTA_CONVERGED=1.e-03f;

//----------------------------------------------------------------------------
int vtkPentagonalPrism::EvaluatePosition(double x[3], double closestPoint[3],
                                   int& subId, double pcoords[3], 
                                   double& dist2, double *weights)
{
  int iteration, converged;
  double  params[3];
  double  fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  double  d, pt[3];
  double derivs[30];

  // set initial position for Newton's method
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2]=0.5;

  //  enter iteration loop
  for (iteration=converged=0;
       !converged && (iteration < VTK_PENTA_MAX_ITERATION); iteration++) 
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
    if ( ((fabs(pcoords[0]-params[0])) < VTK_PENTA_CONVERGED) &&
         ((fabs(pcoords[1]-params[1])) < VTK_PENTA_CONVERGED) &&
         ((fabs(pcoords[2]-params[2])) < VTK_PENTA_CONVERGED) )
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
      this->EvaluateLocation(subId, pc, closestPoint, (double *)w);
      dist2 = vtkMath::Distance2BetweenPoints(closestPoint,x);
      }
    return 0;
    }
}

//----------------------------------------------------------------------------
//
// Compute iso-parametrix interpolation functions
//
void vtkPentagonalPrism::InterpolationFunctions(double pcoords[3], double sf[10])
{
  double r, s, t;
  r = pcoords[0];
  s = pcoords[1];
  t = pcoords[2];

  //First hexagon
  sf[0] = -64./3*r*(r - 0.75)*(r - 1.0)*(s - 0.5)*(s - 1.0)*(t - 1.0);
  sf[1] =  64./3*r*(r - 0.25)*(r - 1.0)*(s - 0.5)*(s - 1.0)*(t - 1.0);
  sf[2] = 4.*r                         *(s - 0.0)*(s - 1.0)*(t - 1.0);
  sf[3] = -2.                          *(s - 0.5)*(s - 0.0)*(t - 1.0);
  sf[4] = -4.*(r - 1.)                 *(s - 0.0)*(s - 1.0)*(t - 1.0);

  //Second hexagon
  sf[5] =  64./3*r*(r - 0.75)*(r - 1.0)*(s - 0.5)*(s - 1.0)*(t - 0.0);
  sf[6] = -64./3*r*(r - 0.25)*(r - 1.0)*(s - 0.5)*(s - 1.0)*(t - 0.0);
  sf[7] = -4*r                         *(s - 0.0)*(s - 1.0)*(t - 0.0);
  sf[8] =  2.                          *(s - 0.5)*(s - 0.0)*(t - 0.0);
  sf[9] =  4*(r - 1.0)                 *(s - 0.0)*(s - 1.0)*(t - 0.0);
}
//----------------------------------------------------------------------------
void vtkPentagonalPrism::InterpolationDerivs(double pcoords[3], double derivs[24])
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
void vtkPentagonalPrism::EvaluateLocation(int& vtkNotUsed(subId), double pcoords[3], double x[3], double *weights)
{
  int i, j;
  double pt[3];

  this->InterpolationFunctions(pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i = 0; i < 10; i++)
    {
    this->Points->GetPoint(i, pt);
    for (j = 0; j < 3; j++)
      {
      x[j] += pt [j] * weights [i];
      }
    }
}

//----------------------------------------------------------------------------
// Returns the closest face to the point specified. Closeness is measured
// parametrically.
int vtkPentagonalPrism::CellBoundary(int vtkNotUsed(subId), double pcoords[3], 
                                     vtkIdList *pts)
{
  int i;

  // define 9 planes that separate regions
  static double normals[9][3] = { 
    {0.0,0.83205,-0.5547}, {-0.639602,-0.639602,-0.426401}, {0.83205,0.0,-0.5547},
    {0.0,0.83205,0.5547}, {-0.639602,-0.639602,0.426401}, {0.83205,0.0,0.5547},
    {-0.707107,0.707107,0.0}, {0.447214,0.894427,0.0}, {0.894427,0.447214,0.0} };
  static double point[3] = {0.333333,0.333333,0.5};
  double vals[9];

  // evaluate 9 plane equations
  for (i=0; i<9; i++)
    {
    vals[i] = normals[i][0]*(pcoords[0]-point[0]) + 
      normals[i][1]*(pcoords[1]-point[1]) + normals[i][2]*(pcoords[2]-point[2]);
    }

  // compare against nine planes in parametric space that divide element
  // into five pieces (each corresponding to a face).
  if ( vals[0] >= 0.0 && vals[1] >= 0.0 && vals[2] >= 0.0 )
    {
    pts->SetNumberOfIds(3); //triangle face
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(2));
    }

  else if ( vals[3] >= 0.0 && vals[4] >= 0.0 && vals[5] >= 0.0 )
    {
    pts->SetNumberOfIds(3); //triangle face
    pts->SetId(0,this->PointIds->GetId(3));
    pts->SetId(1,this->PointIds->GetId(4));
    pts->SetId(2,this->PointIds->GetId(5));
    }

  else if ( vals[0] <= 0.0 && vals[3] <= 0.0 && 
            vals[6] <= 0.0 && vals[7] <= 0.0 )
    {
    pts->SetNumberOfIds(4); //quad face
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
    pts->SetId(2,this->PointIds->GetId(4));
    pts->SetId(3,this->PointIds->GetId(3));
    }

  else if ( vals[1] <= 0.0 && vals[4] <= 0.0 && 
            vals[7] >= 0.0 && vals[8] >= 0.0 )
    {
    pts->SetNumberOfIds(4); //quad face
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(2));
    pts->SetId(2,this->PointIds->GetId(5));
    pts->SetId(3,this->PointIds->GetId(4));
    }

  else //vals[2] <= 0.0 && vals[5] <= 0.0 && vals[8] <= 0.0 && vals[6] >= 0.0
    {
    pts->SetNumberOfIds(4); //quad face
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(0));
    pts->SetId(2,this->PointIds->GetId(3));
    pts->SetId(3,this->PointIds->GetId(5));
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

static int edges[15][2] = { {0,1}, {1,2}, {2,3}, 
                            {3,4}, {4,0}, {5,6}, 
                            {6,7}, {7,8}, {8,9}, 
                            {9,5}, {0,5}, {1,6}, 
                            {2,7}, {3,8}, {4,9} };

static int faces[7][5] = { {0,4,3,2,1}, {5,6,7,8,9}, 
                           {0,1,6,5,-1}, {1,2,7,6,-1}, 
                           {2,3,8,7,-1}, {3,4,9,8,-1}, 
                           {4,0,5,9,-1} };

static int InternalWedges[5][6] = {{0,1,10,5,6,11},
                                   {1,2,10,6,7,11},
                                   {2,3,10,7,8,11},
                                   {3,4,10,8,9,11},
                                   {4,0,10,9,5,11}};

//----------------------------------------------------------------------------
int *vtkPentagonalPrism::GetEdgeArray(int edgeId)
{
  return edges[edgeId];
}

//----------------------------------------------------------------------------
vtkCell *vtkPentagonalPrism::GetEdge(int edgeId)
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
int *vtkPentagonalPrism::GetFaceArray(int faceId)
{
  return faces[faceId];
}
//----------------------------------------------------------------------------
vtkCell *vtkPentagonalPrism::GetFace(int faceId)
{
  int *verts;

  verts = faces[faceId];

  if ( verts[4] != -1 ) // polys cell
    {
    // load point id's
    this->Polygon->PointIds->SetId(0,this->PointIds->GetId(verts[0]));
    this->Polygon->PointIds->SetId(1,this->PointIds->GetId(verts[1]));
    this->Polygon->PointIds->SetId(2,this->PointIds->GetId(verts[2]));
    this->Polygon->PointIds->SetId(3,this->PointIds->GetId(verts[3]));
    this->Polygon->PointIds->SetId(4,this->PointIds->GetId(verts[4]));

    // load coordinates
    this->Polygon->Points->SetPoint(0,this->Points->GetPoint(verts[0]));
    this->Polygon->Points->SetPoint(1,this->Points->GetPoint(verts[1]));
    this->Polygon->Points->SetPoint(2,this->Points->GetPoint(verts[2]));
    this->Polygon->Points->SetPoint(3,this->Points->GetPoint(verts[3]));
    this->Polygon->Points->SetPoint(4,this->Points->GetPoint(verts[4]));

    return this->Polygon;
    }
  else
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
}
//----------------------------------------------------------------------------
// 
// Intersect prism faces against line. Each prism face is a quadrilateral.
//
int vtkPentagonalPrism::IntersectWithLine(double p1[3], double p2[3], double tol,
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
            pcoords[0] = 0.0; pcoords[0] = pc[0]; pcoords[1] = 0.0;
            break;

          case 1:
            pcoords[0] = 1.0; pcoords[0] = pc[0]; pcoords[1] = 0.0;
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
int vtkPentagonalPrism::Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts)
{
  int p[4], i;

  ptIds->Reset();
  pts->Reset();

  // Create five tetrahedron. Triangulation varies depending upon index. This
  // is necessary to insure compatible voxel triangulations.
  if ( (index % 2) )
    {
    p[0] = 0; p[1] = 1; p[2] = 4; p[3] = 3;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 1; p[1] = 4; p[2] = 6; p[3] = 5;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 1; p[1] = 4; p[2] = 3; p[3] = 6;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 1; p[1] = 3; p[2] = 2; p[3] = 6;
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
    p[0] = 2; p[1] = 1; p[2] = 0; p[3] = 5;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 0; p[1] = 2; p[2] = 7; p[3] = 3;
    for ( i=0; i < 4; i++ )
      {
      ptIds->InsertNextId(this->PointIds->GetId(p[i]));
      pts->InsertNextPoint(this->Points->GetPoint(p[i]));
      }

    p[0] = 2; p[1] = 5; p[2] = 7; p[3] = 6;
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
//
// Compute derivatives in x-y-z directions. Use chain rule in combination
// with interpolation function derivatives.
//
void vtkPentagonalPrism::Derivatives(int vtkNotUsed(subId), double pcoords[3], 
                                double *values, int dim, double *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[30], sum[3];
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
void vtkPentagonalPrism::JacobianInverse(double pcoords[3], double **inverse,
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
void vtkPentagonalPrism::GetEdgePoints (int edgeId, int *&pts)
{
  pts = this->GetEdgeArray(edgeId);
}

//----------------------------------------------------------------------------
void vtkPentagonalPrism::GetFacePoints (int faceId, int *&pts)
{
  pts = this->GetFaceArray(faceId);
}

static double vtkPentagonalPrismCellPCoords[30] = {0.25,0.0,0.0, 0.75,0.0,0.0,
                                                   1.0 ,0.5,0.0, 0.5 ,1.0,0.0,
                                                   0.0 ,0.5,0.0, 
                                                   0.25,0.0,1.0, 0.75,0.0,1.0, 
                                                   1.0 ,0.5,1.0, 0.5 ,1.0,1.0,
                                                   0.0 ,0.5,1.0 };

//----------------------------------------------------------------------------
double *vtkPentagonalPrism::GetParametricCoords()
{
  return vtkPentagonalPrismCellPCoords;
}
