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
//
// The Interpolation functions and derivatives were changed in June
// 2015 by Bill Lorensen. These changes follow the formulation in:
// http://dilbert.engr.ucdavis.edu/~suku/nem/papers/polyelas.pdf
// NOTE: An additional copy of this paper is located at:
// http://www.vtk.org/Wiki/File:ApplicationOfPolygonalFiniteElementsInLinearElasticity.pdf

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
//  Method to calculate parametric coordinates in a pentagonal prism
//  from global coordinates
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
      vtkDebugMacro (<<"Determinant incorrect, iteration " << iteration);
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
// See:
// http://dilbert.engr.ucdavis.edu/~suku/nem/papers/polyelas.pdf

void vtkPentagonalPrism::InterpolationFunctions(double pcoords[3],
                                                double weights[10])
{
  // VTK needs parametric coordinates to be between [0,1]. Isoparametric
  // shape functions are formulated between [-1,1]. Here we do a
  // coordinate system conversion from [0,1] to [-1,1].
  double x = 2.0*(pcoords[0]-0.5);
  double y = 2.0*(pcoords[1]-0.5);
  double z = pcoords[2]; // z is from 0 to 1

  // From Appendix A.1 Pentagonal reference element (n = 5)
  double b = 87.05 - 12.7004 * x * x - 12.7004 * y * y;

  double a[5];
  a[0] =
    -0.092937 * (3.23607 + 4 * x) *
    (-3.80423 + 3.80423 * x - 2.76393 * y) *
    (15.2169 + 5.81234 * x + 17.8885 * y);
  a[1] =
    - 0.0790569 * (3.80423 - 3.80423 * x - 2.76393 * y) *
    (-3.80423 + 3.80423 * x - 2.76393 * y ) *
    (15.2169 + 5.81234 * x + 17.8885 * y );
  a[2] =
    - 0.0790569 * (15.2169 + 5.81234 * x - 17.8885 * y) *
    (3.80423 - 3.80423 * x - 2.76393 * y) *
    (-3.80423 + 3.80423 * x - 2.76393 * y);
  a[3] =
    0.092937 * (3.23607 + 4.0 * x) *
    (15.2169 + 5.81234 * x - 17.8885 * y) *
    (3.80423 - 3.80423 * x - 2.76393 * y);
  a[4] =
    0.0232343 * (3.23607 + 4.0 * x) *
    (15.2169 + 5.81234 * x - 17.8885 * y) *
    (15.2169 + 5.81234 * x + 17.8885 * y);

  for (int i = 0; i < 5; ++i)
  {
    weights[i]     = -(a[i] / b) * (z - 1.0);
    weights[i + 5] = (a[i] / b) * (z - 0.0);
  }
}

//----------------------------------------------------------------------------
//
// Compute iso-parametric interpolation derivatives
// See:
// http://dilbert.engr.ucdavis.edu/~suku/nem/papers/polyelas.pdf
//
void vtkPentagonalPrism::InterpolationDerivs(double pcoords[3], double derivs[30])
{
  // VTK needs parametric coordinates to be between [0,1]. Isoparametric
  // shape functions are formulated between [-1,1]. Here we do a
  // coordinate system conversion from [0,1] to [-1,1].
  double x = 2.0*(pcoords[0]-0.5);
  double y = 2.0*(pcoords[1]-0.5);
  double z = pcoords[2];  // z is from 0 to 1

  double dd[20];

  // x-derivatives
  // First pentagon
  double x2 = x * x;
  double y2 = y * y;
  double denom = (-12.7004*x2 - 12.7004*y2 + 87.05);
  double denom2 = denom * denom;

  // Please excuse the line length. This code was generated using the
  // symbolic math package SymPy. (http://www.sympy.org)

  dd[0] =  25.4008*x*(-0.371748*x - 0.30075063759)*(3.80423*x - 2.76393*y - 3.80423)*(5.81234*x + 17.8885*y + 15.2169)/denom2 + 5.81234*(-0.371748*x - 0.30075063759)*(3.80423*x - 2.76393*y - 3.80423)/denom + 3.80423*(-0.371748*x - 0.30075063759)*(5.81234*x + 17.8885*y + 15.2169)/denom - 0.371748*(3.80423*x - 2.76393*y - 3.80423)*(5.81234*x + 17.8885*y + 15.2169)/denom ;

  dd[1] =  25.4008*x*(0.300750630687*x + 0.218507737617*y - 0.300750630687)*(3.80423*x - 2.76393*y - 3.80423)*(5.81234*x + 17.8885*y + 15.2169)/denom2 + 5.81234*(0.300750630687*x + 0.218507737617*y - 0.300750630687)*(3.80423*x - 2.76393*y - 3.80423)/denom + 3.80423*(0.300750630687*x + 0.218507737617*y - 0.300750630687)*(5.81234*x + 17.8885*y + 15.2169)/denom + 0.300750630687*(3.80423*x - 2.76393*y - 3.80423)*(5.81234*x + 17.8885*y + 15.2169)/denom ;

  dd[2] =  25.4008*x*(-3.80423*x - 2.76393*y + 3.80423)*(-0.459505582146*x + 1.41420935565*y - 1.20300094161)*(3.80423*x - 2.76393*y - 3.80423)/denom2 + 3.80423*(-3.80423*x - 2.76393*y + 3.80423)*(-0.459505582146*x + 1.41420935565*y - 1.20300094161)/denom - 0.459505582146*(-3.80423*x - 2.76393*y + 3.80423)*(3.80423*x - 2.76393*y - 3.80423)/denom - 3.80423*(-0.459505582146*x + 1.41420935565*y - 1.20300094161)*(3.80423*x - 2.76393*y - 3.80423)/denom ;

  dd[3] =  25.4008*x*(0.371748*x + 0.30075063759)*(-3.80423*x - 2.76393*y + 3.80423)*(5.81234*x - 17.8885*y + 15.2169)/denom2 + 5.81234*(0.371748*x + 0.30075063759)*(-3.80423*x - 2.76393*y + 3.80423)/denom - 3.80423*(0.371748*x + 0.30075063759)*(5.81234*x - 17.8885*y + 15.2169)/denom + 0.371748*(-3.80423*x - 2.76393*y + 3.80423)*(5.81234*x - 17.8885*y + 15.2169)/denom ;

  dd[4] =  25.4008*x*(0.0929372*x + 0.075187821201)*(5.81234*x - 17.8885*y + 15.2169)*(5.81234*x + 17.8885*y + 15.2169)/denom2 + 5.81234*(0.0929372*x + 0.075187821201)*(5.81234*x - 17.8885*y + 15.2169)/denom + 5.81234*(0.0929372*x + 0.075187821201)*(5.81234*x + 17.8885*y + 15.2169)/denom + 0.0929372*(5.81234*x - 17.8885*y + 15.2169)*(5.81234*x + 17.8885*y + 15.2169)/denom ;

  // y-derivatives
  //First pentagon
  dd[10] =  25.4008*y*(-0.371748*x - 0.30075063759)*(3.80423*x - 2.76393*y - 3.80423)*(5.81234*x + 17.8885*y + 15.2169)/denom2 + 17.8885*(-0.371748*x - 0.30075063759)*(3.80423*x - 2.76393*y - 3.80423)/denom - 2.76393*(-0.371748*x - 0.30075063759)*(5.81234*x + 17.8885*y + 15.2169)/denom ;

  dd[11] =  25.4008*y*(0.300750630687*x + 0.218507737617*y - 0.300750630687)*(3.80423*x - 2.76393*y - 3.80423)*(5.81234*x + 17.8885*y + 15.2169)/denom2 + 17.8885*(0.300750630687*x + 0.218507737617*y - 0.300750630687)*(3.80423*x - 2.76393*y - 3.80423)/denom - 2.76393*(0.300750630687*x + 0.218507737617*y - 0.300750630687)*(5.81234*x + 17.8885*y + 15.2169)/denom + 0.218507737617*(3.80423*x - 2.76393*y - 3.80423)*(5.81234*x + 17.8885*y + 15.2169)/denom ;

  dd[12] =  25.4008*y*(-3.80423*x - 2.76393*y + 3.80423)*(-0.459505582146*x + 1.41420935565*y - 1.20300094161)*(3.80423*x - 2.76393*y - 3.80423)/denom2 - 2.76393*(-3.80423*x - 2.76393*y + 3.80423)*(-0.459505582146*x + 1.41420935565*y - 1.20300094161)/denom + 1.41420935565*(-3.80423*x - 2.76393*y + 3.80423)*(3.80423*x - 2.76393*y - 3.80423)/denom - 2.76393*(-0.459505582146*x + 1.41420935565*y - 1.20300094161)*(3.80423*x - 2.76393*y - 3.80423)/denom ;

  dd[13] =  25.4008*y*(0.371748*x + 0.30075063759)*(-3.80423*x - 2.76393*y + 3.80423)*(5.81234*x - 17.8885*y + 15.2169)/denom2 - 17.8885*(0.371748*x + 0.30075063759)*(-3.80423*x - 2.76393*y + 3.80423)/denom - 2.76393*(0.371748*x + 0.30075063759)*(5.81234*x - 17.8885*y + 15.2169)/denom ;

  dd[14] =  25.4008*y*(0.0929372*x + 0.075187821201)*(5.81234*x - 17.8885*y + 15.2169)*(5.81234*x + 17.8885*y + 15.2169)/denom2 + 17.8885*(0.0929372*x + 0.075187821201)*(5.81234*x - 17.8885*y + 15.2169)/denom - 17.8885*(0.0929372*x + 0.075187821201)*(5.81234*x + 17.8885*y + 15.2169)/denom ;

  // z-derivatives
  // First pentagon
  double b = 87.05 - 12.7004 * x * x - 12.7004 * y * y;
  dd[15] =
    -0.092937 * (3.23607 + 4 * x) *
    (-3.80423 + 3.80423 * x - 2.76393 * y) *
    (15.2169 + 5.81234 * x + 17.8885 * y) / b;
  dd[16] =
    - 0.0790569 * (3.80423 - 3.80423 * x - 2.76393 * y) *
    (-3.80423 + 3.80423 * x - 2.76393 * y ) *
    (15.2169 + 5.81234 * x + 17.8885 * y ) / b;
  dd[17] =
    - 0.0790569 * (15.2169 + 5.81234 * x - 17.8885 * y) *
    (3.80423 - 3.80423 * x - 2.76393 * y) *
    (-3.80423 + 3.80423 * x - 2.76393 * y) / b;
  dd[18] =
    0.092937 * (3.23607 + 4.0 * x) *
    (15.2169 + 5.81234 * x - 17.8885 * y) *
    (3.80423 - 3.80423 * x - 2.76393 * y) / b;
  dd[19] =
    0.0232343 * (3.23607 + 4.0 * x) *
    (15.2169 + 5.81234 * x - 17.8885 * y) *
    (15.2169 + 5.81234 * x + 17.8885 * y) / b;

  for (int i = 0; i < 5; ++i)
  {
    derivs[i]      = -dd[i]      * (z - 1.0);  // x deriv first pentagon
    derivs[i + 5]  =  dd[i]      * (z + 0.0);  // x deriv second pentagon
    derivs[i + 10] = -dd[i + 10] * (z - 1.0);  // y deriv first pentagon
    derivs[i + 15] =  dd[i + 10] * (z + 0.0);  // y deriv second pentagon
    derivs[i + 20] = -dd[i + 15];              // z deriv first pentagon
    derivs[i + 25] =  dd[i + 15];              // z deriv second pentagon
  }

  // We compute derivatives in [-1; 1] but we need them in [ 0; 1]
  for(int i = 0; i < 30; i++)
  {
    derivs[i] *= 2;
  }
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

static int faces[7][6] = { {0,4,3,2,1,-1}, {5,6,7,8,9,-1},
                           {0,1,6,5,-1,-1}, {1,2,7,6,-1,-1},
                           {2,3,8,7,-1,-1}, {3,4,9,8,-1,-1},
                           {4,0,5,9,-1,-1} };

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

  int min = vtkMath::Min(pts->GetId( 0 ), pts->GetId( 1 ));
  int max = vtkMath::Max(pts->GetId( 0 ), pts->GetId( 1 ));

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
  // mathematically dot must be >= zero but, surprise surprise, it can actually
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

// See:
// http://dilbert.engr.ucdavis.edu/~suku/nem/papers/polyelas.pdf

static double vtkPentagonalPrismCellPCoords[30] = {
  0.654508, 0.975528, 0,
  0.0954915, 0.793893, 0,
  0.0954915, 0.206107, 0,
  0.654508, 0.0244717, 0,
  1, 0.5, 0,
  0.654508, 0.975528, 1,
  0.0954915, 0.793893, 1,
  0.0954915, 0.206107, 1,
  0.654508, 0.0244717, 1,
  1, 0.5, 1};

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

