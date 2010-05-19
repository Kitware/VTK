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
//.SECTION Thanks
// Thanks to Philippe Guerville who developed this class. <br>
// Thanks to Charles Pignerol (CEA-DAM, France) who ported this class under
// VTK 4.
// Thanks to Jean Favre (CSCS, Switzerland) who contributed to integrate this
// class in VTK. <br>
// Please address all comments to Jean Favre (jfavre at cscs.ch).

#include "vtkPentagonalPrism.h"

#include "vtkObjectFactory.h"
#include "vtkLine.h"
#include "vtkQuad.h"
#include "vtkPolygon.h"
#include "vtkTriangle.h"
#include "vtkMath.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkPentagonalPrism);

static const double VTK_DIVERGED = 1.e6;

//----------------------------------------------------------------------------
// Construct the prism with ten points.
vtkPentagonalPrism::vtkPentagonalPrism()
{
  int i;
  this->Points->SetNumberOfPoints(10);
  this->PointIds->SetNumberOfIds(10);

  for (i = 0; i < 10; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }

  this->Line = vtkLine::New();
  this->Quad = vtkQuad::New();
  this->Triangle = vtkTriangle::New();
  this->Polygon = vtkPolygon::New();
  this->Polygon->PointIds->SetNumberOfIds(5);
  this->Polygon->Points->SetNumberOfPoints(5);

  for (i = 0; i < 5; i++)
    {
    this->Polygon->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->Polygon->PointIds->SetId(i,0);
    }
}

//----------------------------------------------------------------------------
vtkPentagonalPrism::~vtkPentagonalPrism()
{
  this->Line->Delete();
  this->Quad->Delete();
  this->Triangle->Delete();
  this->Polygon->Delete();
}

//
//  Method to calculate parametric coordinates in an ten noded
//  linear prism element from global coordinates.
//
static const int VTK_PENTA_MAX_ITERATION=10;
static const double VTK_PENTA_CONVERGED=1.e-03;

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
    for (i=0; i<10; i++)
      {
      this->Points->GetPoint(i, pt);
      for (j=0; j<3; j++)
        {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i+10];
        tcol[j] += pt[j] * derivs[i+20];
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
    double pc[3], w[10];
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
//
// Compute iso-parametric interpolation functions
//

// see vtkPentagonalPrismCellPCoords for V#i values:
// The general idea is that for Point #0 (V1,V1,0) the shape function should be
// 0 on the 4 other node. So expr of the line passing through points
// (x1,y1) and (x2,y2) is as follow:
// (x1-x2)*y - (y1-y2)*x - (x1*y2 - x2*y1) = 0
// x(i):=1/2+1/2*Cos( Pi + Pi/4 + i*2*Pi/5)
// y(i):=1/2+1/2*Sin( Pi + Pi/4 + i*2*Pi/5)
// For instance EXPRA is x(2)-x(1)
//              EXPRB is y(2)-y(1) (== x(4)-x(3))
//              EXPRC is x(1)*y(2)-x(2)*y(1)
//              EXPRD is x(2)-x(3) (because of sign)
//              EXPRE is x(2)*y(3)-x(3)*y(2)
//              EXPRF is x(0)-x(4)
//              EXPRG is y(4)-y(0)
//              EXPRH is x(0)*y(4)-x(4)*y(0)
// EXPRN was deducted to normalize the function
#define EXPRA 0.26684892042779546;
#define EXPRB 0.52372049461429937;
#define EXPRC 0.36619991616704034;
#define EXPRD 0.41562693777745341;
#define EXPRE 0.65339106685124182;
#define EXPRF 0.091949871500910163;
#define EXPRG 0.58054864046304711;
#define EXPRH 0.098485126908190265;
#define EXPRN 9.2621670111997307;

void vtkPentagonalPrism::InterpolationFunctions(double pcoords[3], double sf[10])
{
  double r, s, t;
  r = pcoords[0];
  s = pcoords[1];
  t = pcoords[2];

  const double a = EXPRA;
  const double b = EXPRB;
  const double c = EXPRC;
  const double d = EXPRD;
  const double e = EXPRE;
  const double f = EXPRF;
  const double g = EXPRG;
  const double h = EXPRH;
  const double n = EXPRN;

  //First pentagon
  sf[0] = -n*(-a*s + b*r - c)*( b*s - a*r - c)*(t - 1.0);
  sf[1] =  n*( d*s + d*r - e)*( f*s + g*r - h)*(t - 1.0);
  sf[2] = -n*( b*s - a*r - c)*(-g*s - f*r + h)*(t - 1.0);
  sf[3] =  n*(-a*s + b*r - c)*( f*s + g*r - h)*(t - 1.0);
  sf[4] = -n*(-g*s - f*r + h)*( d*s + d*r - e)*(t - 1.0);

  //Second pentagon
  sf[5] =  n*(-a*s + b*r - c)*( b*s - a*r - c)*(t - 0.0);
  sf[6] = -n*( d*s + d*r - e)*( f*s + g*r - h)*(t - 0.0);
  sf[7] =  n*( b*s - a*r - c)*(-g*s - f*r + h)*(t - 0.0);
  sf[8] = -n*(-a*s + b*r - c)*( f*s + g*r - h)*(t - 0.0);
  //sf[9] =  n*(-g*s - f*r + h)*( d*s + d*r - e)*(t - 0.0);
  sf[9] = 1. - (sf[0]+sf[1]+sf[2]+sf[3]+sf[4]+sf[5]+sf[6]+sf[7]+sf[8]);
}

//----------------------------------------------------------------------------
//
// Compute iso-parametric interpolation derivatives
//
void vtkPentagonalPrism::InterpolationDerivs(double pcoords[3], double derivs[30])
{
  double r, s, t;
  r = pcoords[0];
  s = pcoords[1];
  t = pcoords[2];

  const double a = EXPRA;
  const double b = EXPRB;
  const double c = EXPRC;
  const double d = EXPRD;
  const double e = EXPRE;
  const double f = EXPRF;
  const double g = EXPRG;
  const double h = EXPRH;
  const double n = EXPRN;

  // r-derivatives
  //First pentagon
  derivs[0] = -n*(-2*a*b*r + (a*a + b*b)*s + a*c - b*c)*(t - 1.0);
  derivs[1] =  n*( 2*d*g*r + d*(f + g)*s - d*h - e*g)*(t - 1.0);
  derivs[2] = -n*( 2*a*f*r + (a*g - b*f)*s - a*h + c*f)*(t - 1.0);
  derivs[3] =  n*( 2*b*g*r + (b*f - a*g)*s - b*h - c*g)*(t - 1.0);
  derivs[4] = -n*(-2*d*f*r - d*(f + g)*s + d*h + e*f)*(t - 1.0);
  //Second pentagon
  derivs[5] =  n*(-2*a*b*r + (a*a + b*b)*s + a*c - b*c)*(t - 0.0);
  derivs[6] = -n*( 2*d*g*r + d*(f + g)*s - d*h - e*g)*(t - 0.0);
  derivs[7] =  n*( 2*a*f*r + (a*g - b*f)*s - a*h + c*f)*(t - 0.0);
  derivs[8] = -n*( 2*b*g*r + (b*f - a*g)*s - b*h - c*g)*(t - 0.0);
  //derivs[9] =  n*(-2*d*f*r - d*(f + g)*s + d*h + e*f)*(t - 0.0);
  derivs[9] = -(derivs[0]+derivs[1]+derivs[2]+derivs[3]+derivs[4]+derivs[5]
    +derivs[6]+derivs[7]+derivs[8]);

  // s-derivatives
  //First pentagon
  derivs[10] = -n*(-2*a*b*s + (a*a + b*b)*r + a*c - b*c)*(t - 1.0);
  derivs[11] =  n*( 2*d*f*s + d*(f + g)*r - d*h - e*f)*(t - 1.0);
  derivs[12] = -n*(-2*b*g*s + (a*g - b*f)*r + b*h + c*g)*(t - 1.0);
  derivs[13] =  n*(-2*a*f*s + (b*f - a*g)*r + a*h - c*f)*(t - 1.0);
  derivs[14] = -n*(-2*d*g*s - d*(f + g)*r + d*h + e*g)*(t - 1.0);
  //Second pentagon
  derivs[15] =  n*(-2*a*b*s + (a*a + b*b)*r + a*c - b*c)*(t - 0.0);
  derivs[16] = -n*( 2*d*f*s + d*(f + g)*r - d*h - e*f)*(t - 0.0);
  derivs[17] =  n*(-2*b*g*s + (a*g - b*f)*r + b*h + c*g)*(t - 0.0);
  derivs[18] = -n*(-2*a*f*s + (b*f - a*g)*r + a*h - c*f)*(t - 0.0);
  //derivs[19] =  n*(-2*d*g*s - d*(f + g)*r + d*h + e*g)*(t - 0.0);
  derivs[19] = -(derivs[10]+derivs[11]+derivs[12]+derivs[13]+derivs[14]+derivs[15]
    +derivs[16]+derivs[17]+derivs[18]);

  // t-derivatives
  //First pentagon
  derivs[20] = -n*(-a*s + b*r - c)*( b*s - a*r - c);
  derivs[21] =  n*( d*s + d*r - e)*( f*s + g*r - h);
  derivs[22] = -n*( b*s - a*r - c)*(-g*s - f*r + h);
  derivs[23] =  n*(-a*s + b*r - c)*( f*s + g*r - h);
  derivs[24] = -n*(-g*s - f*r + h)*( d*s + d*r - e);
  //Second pentagon
  derivs[25] =  n*(-a*s + b*r - c)*( b*s - a*r - c);
  derivs[26] = -n*( d*s + d*r - e)*( f*s + g*r - h);
  derivs[27] =  n*( b*s - a*r - c)*(-g*s - f*r + h);
  derivs[28] = -n*(-a*s + b*r - c)*( f*s + g*r - h);
  //derivs[29] =  n*(-g*s - f*r + h)*( d*s + d*r - e);
  derivs[29] = -(derivs[20]+derivs[21]+derivs[22]+derivs[23]+derivs[24]+derivs[25]
    +derivs[26]+derivs[27]+derivs[28]);
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
static int edges[15][2] = { {0,1}, {1,2}, {2,3},
                            {3,4}, {4,0}, {5,6},
                            {6,7}, {7,8}, {8,9},
                            {9,5}, {0,5}, {1,6},
                            {2,7}, {3,8}, {4,9} };

static int faces[7][5] = { {0,4,3,2,1}, {5,6,7,8,9},
                           {0,1,6,5,-1}, {1,2,7,6,-1},
                           {2,3,8,7,-1}, {3,4,9,8,-1},
                           {4,0,5,9,-1} };

#define VTK_MAX(a,b)    (((a)>(b))?(a):(b))
#define VTK_MIN(a,b)    (((a)<(b))?(a):(b))

//----------------------------------------------------------------------------
// Returns the closest face to the point specified. Closeness is measured
// parametrically.
int vtkPentagonalPrism::CellBoundary(int subId, double pcoords[3],
                                     vtkIdList *pts)
{
  // load coordinates
  double *points = this->GetParametricCoords();
  for(int i=0;i<5;i++)
    {
    this->Polygon->PointIds->SetId(i, i);
    this->Polygon->Points->SetPoint(i, &points[3*i]);
    }

  this->Polygon->CellBoundary( subId, pcoords, pts);

  int min = VTK_MIN(pts->GetId( 0 ), pts->GetId( 1 ));
  int max = VTK_MAX(pts->GetId( 0 ), pts->GetId( 1 ));

  //Base on the edge find the quad that correspond:
  int index;
  if( (index = (max - min)) > 1)
    {
    index = 6;
    }
  else
    {
    index += min + 1;
    }

  double a[3], b[3], u[3], v[3];
  this->Polygon->Points->GetPoint(pts->GetId( 0 ), a);
  this->Polygon->Points->GetPoint(pts->GetId( 1 ), b);
  u[0] = b[0] - a[0];
  u[1] = b[1] - a[1];
  v[0] = pcoords[0] - a[0];
  v[1] = pcoords[1] - a[1];

  double dot = vtkMath::Dot2D(v, u);
  double uNorm = vtkMath::Norm2D( u );
  if (uNorm)
    {
    dot /= uNorm;
    }
  dot = (v[0]*v[0] + v[1]*v[1]) - dot*dot;
  // mathematically dot must be >= zero but, suprise suprise, it can actually
  // be negative
  if (dot > 0)
    {
    dot = sqrt( dot );
    }
  else
    {
    dot = 0;
    }
  int *verts;

  if(pcoords[2] < 0.5)
    {
    //could be closer to face 1
    //compare that distance to the distance to the quad.

    if(dot < pcoords[2])
      {
      //We are closer to the quad face
      verts = faces[index];
      for(int i=0; i<4; i++)
        {
        pts->InsertId(i, verts[i]);
        }
      }
    else
      {
      //we are closer to the penta face 1
      for(int i=0; i<5; i++)
        {
        pts->InsertId(i, faces[0][i]);
        }
      }
    }
  else
    {
    //could be closer to face 2
    //compare that distance to the distance to the quad.

    if(dot < (1. - pcoords[2]) )
      {
      //We are closer to the quad face
      verts = faces[index];
      for(int i=0; i<4; i++)
        {
        pts->InsertId(i, verts[i]);
        }
      }
    else
      {
      //we are closer to the penta face 2
      for(int i=0; i<5; i++)
        {
        pts->InsertId(i, faces[1][i]);
        }
      }
    }

  // determine whether point is inside of hexagon
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
  double pt1[3], pt2[3], pt3[3], pt4[3], pt5[3];
  double tTemp;
  double pc[3], xTemp[3], dist2, weights[10];
  int faceNum;

  t = VTK_DOUBLE_MAX;

  //first intersect the penta faces
  for (faceNum=0; faceNum<2; faceNum++)
    {
    this->Points->GetPoint(faces[faceNum][0], pt1);
    this->Points->GetPoint(faces[faceNum][1], pt2);
    this->Points->GetPoint(faces[faceNum][2], pt3);
    this->Points->GetPoint(faces[faceNum][3], pt4);
    this->Points->GetPoint(faces[faceNum][4], pt5);

    this->Quad->Points->SetPoint(0,pt1);
    this->Quad->Points->SetPoint(1,pt2);
    this->Quad->Points->SetPoint(2,pt3);
    this->Quad->Points->SetPoint(3,pt4);

    this->Triangle->Points->SetPoint(0,pt4);
    this->Triangle->Points->SetPoint(1,pt5);
    this->Triangle->Points->SetPoint(2,pt1);

    if ( this->Quad->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId) ||
         this->Triangle->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId) )
      {
      intersection = 1;
      if ( tTemp < t )
        {
        t = tTemp;
        x[0] = xTemp[0]; x[1] = xTemp[1]; x[2] = xTemp[2];
        switch (faceNum)
          {
          case 0:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 0.0;
            break;

          case 1:
            pcoords[0] = pc[0]; pcoords[1] = pc[1]; pcoords[2] = 1.0;
            break;
          }
        }
      }
    }

  //now intersect the _5_ quad faces
  for (faceNum=2; faceNum<5; faceNum++)
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
        this->EvaluatePosition(x, xTemp, subId, pcoords, dist2, weights);
        }
      }
    }

  return intersection;
}

//----------------------------------------------------------------------------
int vtkPentagonalPrism::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, vtkPoints *pts)
{
  ptIds->Reset();
  pts->Reset();

  for ( int i=0; i < 4; i++ )
    {
    ptIds->InsertId(i,this->PointIds->GetId(i));
    pts->InsertPoint(i,this->Points->GetPoint(i));
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
    for ( i=0; i < 10; i++) //loop over interp. function derivatives
      {
      sum[0] += functionDerivs[i] * values[dim*i + k];
      sum[1] += functionDerivs[10 + i] * values[dim*i + k];
      sum[2] += functionDerivs[20 + i] * values[dim*i + k];
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

  for ( j=0; j < 10; j++ )
    {
    this->Points->GetPoint(j, x);
    for ( i=0; i < 3; i++ )
      {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[10 + j];
      m2[i] += x[i] * derivs[20 + j];
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

// How to find the points for the pentagon:
// The points for the iso parametric pentagon have to be properly chosen so that
// the inverse Jacobian is defined.
// To be regular the points have to on the circle, center (1/2,1/2) with radius
// sqrt(2)/2
// Then since there is an odd number of points they have to be simmetric
// to the first bisector
// Thus I pick the first point to be on this dividing line.
// We can then express point i (0, 4) to be:
// Vi_x = CenterOfCircle + 1/2 ( cos( pi + pi/4 + i*2*pi/5) )
// Vi_y = CenterOfCircle + 1/2 ( sin( pi + pi/4 + i*2*pi/5) )

#define V1 0.14644660940672624
#define V2 0.72699524986977337
#define V3 0.054496737905816071
#define V4 0.99384417029756889
#define V5 0.57821723252011548

static double vtkPentagonalPrismCellPCoords[30] = {
V1 , V1 , 0.0,
V2 , V3 , 0.0,
V4 , V5 , 0.0,
V5 , V4 , 0.0,
V3 , V2 , 0.0,
V1 , V1 , 1.0,
V2 , V3 , 1.0,
V4 , V5 , 1.0,
V5 , V4 , 1.0,
V3 , V2 , 1.0};

//----------------------------------------------------------------------------
double *vtkPentagonalPrism::GetParametricCoords()
{
  return vtkPentagonalPrismCellPCoords;
}

//----------------------------------------------------------------------------
void vtkPentagonalPrism::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Quad:\n";
  this->Quad->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Polygon:\n";
  this->Polygon->PrintSelf(os,indent.GetNextIndent());
}

