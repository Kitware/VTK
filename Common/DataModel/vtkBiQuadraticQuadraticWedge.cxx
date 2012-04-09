/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiQuadraticQuadraticWedge.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//Thanks to Soeren Gebbert  who developed this class and
//integrated it into VTK 5.0.

#include "vtkBiQuadraticQuadraticWedge.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkWedge.h"
#include "vtkMath.h"
#include "vtkQuadraticEdge.h"
#include "vtkBiQuadraticQuad.h"
#include "vtkQuadraticTriangle.h"
#include "vtkPoints.h"

#include <assert.h>

vtkStandardNewMacro (vtkBiQuadraticQuadraticWedge);

//----------------------------------------------------------------------------
// Construct the biquadratic quadratic wedge with 18 points

vtkBiQuadraticQuadraticWedge::vtkBiQuadraticQuadraticWedge ()
{
  this->Points->SetNumberOfPoints (18);
  this->PointIds->SetNumberOfIds (18);
  for (int i = 0; i < 18; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }

  this->Edge = vtkQuadraticEdge::New ();
  this->Face = vtkBiQuadraticQuad::New ();
  this->TriangleFace = vtkQuadraticTriangle::New ();
  this->Wedge = vtkWedge::New ();

  this->Scalars = vtkDoubleArray::New ();
  this->Scalars->SetNumberOfTuples (6); //Number of vertices from a linear wedge

}

//----------------------------------------------------------------------------
vtkBiQuadraticQuadraticWedge::~vtkBiQuadraticQuadraticWedge ()
{
  this->Edge->Delete ();
  this->Face->Delete ();
  this->TriangleFace->Delete ();
  this->Wedge->Delete ();
  this->Scalars->Delete ();
}

//----------------------------------------------------------------------------
// We are using 8 linear wedge
static int LinearWedges[8][6] = { {0,6,8,12,15,17},
                                  {6,7,8,15,16,17},
                                  {6,1,7,15,13,16},
                                  {8,7,2,17,16,14},
                                  {12,15,17,3,9,11},
                                  {15,16,17,9,10,11},
                                  {15,13,16,9,4,10},
                                  {17,16,14,11,10,5} };

// We use 2 quadratic triangles and 3 quadratic-linear quads
static int WedgeFaces[5][9] = {
    {0, 1, 2,  6,  7,  8,  0,  0,  0},  // first quad triangle
    {3, 5, 4, 11, 10,  9,  0,  0,  0},  // second quad triangle
    {0, 3, 4,  1, 12,  9, 13,  6, 15},  // 1. biquad quad
    {1, 4, 5,  2, 13, 10, 14,  7, 16},  // 2. biquad quad
    {2, 5, 3,  0, 14, 11, 12,  8, 17}   // 3. biquad quad
};

// We have 9 quadratic edges
static int WedgeEdges[9][3] = {
    {0, 1,  6}, {1, 2,  7}, {2, 0,  8},
    {3, 4,  9}, {4, 5, 10}, {5, 3, 11},
    {0, 3, 12}, {1, 4, 13}, {2, 5, 14}
};
//----------------------------------------------------------------------------
int *vtkBiQuadraticQuadraticWedge::GetEdgeArray(int edgeId)
{
  return WedgeEdges[edgeId];
}
//----------------------------------------------------------------------------
int *vtkBiQuadraticQuadraticWedge::GetFaceArray(int faceId)
{
  return WedgeFaces[faceId];
}

//----------------------------------------------------------------------------
vtkCell *vtkBiQuadraticQuadraticWedge::GetEdge(int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 8 ? 8 : edgeId));

  //We have 9 quadratic edges
  for (int i = 0; i < 3; i++)
    {
    this->Edge->PointIds->SetId (i, this->PointIds->GetId (WedgeEdges[edgeId][i]));
    this->Edge->Points->SetPoint (i, this->Points->GetPoint (WedgeEdges[edgeId][i]));
    }

  return this->Edge;
}

//----------------------------------------------------------------------------
vtkCell *vtkBiQuadraticQuadraticWedge::GetFace(int faceId)
{
  faceId = (faceId < 0 ? 0 : (faceId > 4 ? 4 : faceId));

  // load point id's and coordinates
  // be careful with the last two:
  if (faceId < 2)
    {
    for (int i = 0; i < 6; i++)
      {
      this->TriangleFace->PointIds->SetId (i, this->PointIds->GetId (WedgeFaces[faceId][i]));
      this->TriangleFace->Points->SetPoint (i, this->Points->GetPoint (WedgeFaces[faceId][i]));
      }
    return this->TriangleFace;
    }
  else
    {
    for (int i = 0; i < 9; i++)
      {
      this->Face->PointIds->SetId (i, this->PointIds->GetId (WedgeFaces[faceId][i]));
      this->Face->Points->SetPoint (i, this->Points->GetPoint (WedgeFaces[faceId][i]));
      }
    return this->Face;
    }
}

//----------------------------------------------------------------------------
static const double VTK_DIVERGED = 1.e6;
static const int VTK_WEDGE_MAX_ITERATION = 20;
static const double VTK_WEDGE_CONVERGED = 1.e-03;

int vtkBiQuadraticQuadraticWedge::EvaluatePosition (double *x,
  double *closestPoint,
  int &subId, double pcoords[3], double &dist2, double *weights)
{
  int iteration, converged;
  double params[3];
  double fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  double d, pt[3];
  double derivs[3 * 18];

  //  set initial position for Newton's method
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2] = 0.5;

  //  enter iteration loop
  for (iteration = converged = 0; !converged && (iteration < VTK_WEDGE_MAX_ITERATION); iteration++)
    {
    //  calculate element interpolation functions and derivatives
    this->InterpolationFunctions (pcoords, weights);
    this->InterpolationDerivs (pcoords, derivs);

    //  calculate newton functions
    for (i = 0; i < 3; i++)
      {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
      }
    for (i = 0; i < 18; i++)
      {
      this->Points->GetPoint (i, pt);
      for (j = 0; j < 3; j++)
        {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i + 18];
        tcol[j] += pt[j] * derivs[i + 36];
        }
      }

    for (i = 0; i < 3; i++)
      {
      fcol[i] -= x[i];
      }

    //  compute determinants and generate improvements
    d = vtkMath::Determinant3x3 (rcol, scol, tcol);
    if (fabs (d) < 1.e-20)
      {
      return -1;
      }

    pcoords[0] = params[0] - 0.5 * vtkMath::Determinant3x3 (fcol, scol, tcol) / d;
    pcoords[1] = params[1] - 0.5 * vtkMath::Determinant3x3 (rcol, fcol, tcol) / d;
    pcoords[2] = params[2] - 0.5 * vtkMath::Determinant3x3 (rcol, scol, fcol) / d;

    //  check for convergence
    if (((fabs (pcoords[0] - params[0])) < VTK_WEDGE_CONVERGED) &&
      ((fabs (pcoords[1] - params[1])) < VTK_WEDGE_CONVERGED) &&
      ((fabs (pcoords[2] - params[2])) < VTK_WEDGE_CONVERGED))
      {
      converged = 1;
      }

    // Test for bad divergence (S.Hirschberg 11.12.2001)
    else if ((fabs (pcoords[0]) > VTK_DIVERGED) ||
      (fabs (pcoords[1]) > VTK_DIVERGED) || (fabs (pcoords[2]) > VTK_DIVERGED))
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
  if (!converged)
    {
    return -1;
    }

  this->InterpolationFunctions (pcoords, weights);

  if (pcoords[0] >= -0.001 && pcoords[0] <= 1.001 &&
    pcoords[1] >= -0.001 && pcoords[1] <= 1.001 && pcoords[2] >= -0.001 && pcoords[2] <= 1.001)
    {
    if (closestPoint)
      {
      closestPoint[0] = x[0];
      closestPoint[1] = x[1];
      closestPoint[2] = x[2];
      dist2 = 0.0;    //inside wedge
      }
    return 1;
    }
  else
    {
    double pc[3], w[18];
    if (closestPoint)
      {
      for (i = 0; i < 3; i++)  //only approximate, not really true for warped hexa
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
      this->EvaluateLocation (subId, pc, closestPoint,
                              static_cast<double *>(w));
      dist2 = vtkMath::Distance2BetweenPoints (closestPoint, x);
      }
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkBiQuadraticQuadraticWedge::EvaluateLocation (int &vtkNotUsed (subId),
  double pcoords[3], double x[3], double *weights)
{
  double pt[3];

  this->InterpolationFunctions (pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (int i = 0; i < 18; i++)
    {
    this->Points->GetPoint (i, pt);
    for (int j = 0; j < 3; j++)
      {
      x[j] += pt[j] * weights[i];
      }
    }
}

//----------------------------------------------------------------------------
int vtkBiQuadraticQuadraticWedge::CellBoundary (int subId,
  double pcoords[3], vtkIdList * pts)
{
  return this->Wedge->CellBoundary (subId, pcoords, pts);
}

//----------------------------------------------------------------------------
void vtkBiQuadraticQuadraticWedge::Contour (double value,
          vtkDataArray * cellScalars,
          vtkIncrementalPointLocator * locator,
          vtkCellArray * verts,
          vtkCellArray * lines,
          vtkCellArray * polys,
          vtkPointData * inPd,
          vtkPointData * outPd, vtkCellData * inCd,
          vtkIdType cellId, vtkCellData * outCd)
{
  //contour each linear wedge separately
  for (int i=0; i<8; i++) //for each wedge
    {
    for (int j=0; j<6; j++) //for each point of wedge
      {
      this->Wedge->Points->SetPoint(j,this->Points->GetPoint(LinearWedges[i][j]));
      this->Wedge->PointIds->SetId(j,this->PointIds->GetId(LinearWedges[i][j]));
      this->Scalars->SetValue(j,cellScalars->GetTuple1(LinearWedges[i][j]));
      }
    this->Wedge->Contour(value,this->Scalars,locator,verts,lines,polys, inPd,outPd,inCd,cellId,outCd);
    }
}

//----------------------------------------------------------------------------
// Clip this biquadratic wedge using scalar value provided. Like contouring,
// except that it cuts the wedge to produce tetrahedra.
void
vtkBiQuadraticQuadraticWedge::Clip (double value, vtkDataArray *cellScalars,
             vtkIncrementalPointLocator * locator, vtkCellArray * tets,
             vtkPointData * inPd, vtkPointData * outPd,
             vtkCellData * inCd, vtkIdType cellId, vtkCellData * outCd, int insideOut)
{
  //contour each linear wedge separately
  for (int i=0; i<8; i++) //for each wedge
    {
    for (int j=0; j<6; j++) //for each of the six vertices of the wedge
      {
      this->Wedge->Points->SetPoint(j,this->Points->GetPoint(LinearWedges[i][j]));
      this->Wedge->PointIds->SetId(j,this->PointIds->GetId(LinearWedges[i][j]));
      this->Scalars->SetValue(j,cellScalars->GetTuple1(LinearWedges[i][j]));
      }
    this->Wedge->Clip(value,this->Scalars,locator,tets,inPd,outPd, inCd,cellId,outCd,insideOut);
    }
}


//----------------------------------------------------------------------------
// Line-hex intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkBiQuadraticQuadraticWedge::IntersectWithLine (double *p1, double *p2,
                                                     double tol, double &t,
                                                     double *x,
                                                     double *pcoords,
                                                     int &subId)
{
  int intersection = 0;
  double tTemp;
  double pc[3], xTemp[3];
  int faceNum;
  int inter;

  t = VTK_DOUBLE_MAX;
  for (faceNum = 0; faceNum < 5; faceNum++)
    {
    // We have 9 nodes on biquad face
    // and 6 on triangle faces
    if (faceNum < 2)
      {
      for (int i = 0; i < 6; i++)
        {
        this->TriangleFace->PointIds->SetId (i, this->PointIds->GetId (WedgeFaces[faceNum][i]));
        this->TriangleFace->Points->SetPoint (i, this->Points->GetPoint (WedgeFaces[faceNum][i]));
        }
      inter = this->TriangleFace->IntersectWithLine (p1, p2, tol, tTemp, xTemp,
                                                     pc, subId);
      }
    else
      {
      for (int i = 0; i < 9; i++)
        {
        this->Face->Points->SetPoint (i, this->Points->GetPoint (WedgeFaces[faceNum][i]));
        }
      inter = this->Face->IntersectWithLine (p1, p2, tol, tTemp, xTemp, pc,
                                             subId);
      }
    if (inter)
      {
      intersection = 1;
      if (tTemp < t)
        {
        t = tTemp;
        x[0] = xTemp[0];
        x[1] = xTemp[1];
        x[2] = xTemp[2];
        switch (faceNum)
          {
          case 0:
            pcoords[0] = 0.0;
            pcoords[1] = pc[1];
            pcoords[2] = pc[0];
            break;

          case 1:
            pcoords[0] = 1.0;
            pcoords[1] = pc[0];
            pcoords[2] = pc[1];
            break;

          case 2:
            pcoords[0] = pc[0];
            pcoords[1] = 0.0;
            pcoords[2] = pc[1];
            break;

          case 3:
            pcoords[0] = pc[1];
            pcoords[1] = 1.0;
            pcoords[2] = pc[0];
            break;

          case 4:
            pcoords[0] = pc[1];
            pcoords[1] = pc[0];
            pcoords[2] = 0.0;
            break;

          case 5:
            pcoords[0] = pc[0];
            pcoords[1] = pc[1];
            pcoords[2] = 1.0;
            break;
          default:
            assert("check:impossible case" && 0); // reaching this line is a bug
            break;
          }
        }
      }
    }
  return intersection;
}

//----------------------------------------------------------------------------
int vtkBiQuadraticQuadraticWedge::Triangulate (int vtkNotUsed (index),
  vtkIdList * ptIds, vtkPoints * pts)
{
  pts->Reset ();
  ptIds->Reset ();

  for (int i = 0; i < 8; i++)
    {
    for (int j = 0; j < 6; j++)
      {
      ptIds->InsertId (6 * i + j, this->PointIds->GetId (LinearWedges[i][j]));
      pts->InsertPoint (6 * i + j, this->Points->GetPoint (LinearWedges[i][j]));
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkBiQuadraticQuadraticWedge::JacobianInverse(double pcoords[3],
  double **inverse, double derivs[54])
{
  int i, j;
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  this->InterpolationDerivs (pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;

  for (i = 0; i < 3; i++)  //initialize matrix
    {
    m0[i] = m1[i] = m2[i] = 0.0;
    }

  for (j = 0; j < 18; j++)
    {
    this->Points->GetPoint (j, x);
    for (i = 0; i < 3; i++)
      {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[18 + j];
      m2[i] += x[i] * derivs[36 + j];
      }
    }

  // now find the inverse
  if (vtkMath::InvertMatrix(m, inverse, 3) == 0)
    {
    vtkErrorMacro (<<"Jacobian inverse not found");
    return;
    }
}

//----------------------------------------------------------------------------
void vtkBiQuadraticQuadraticWedge::Derivatives (int vtkNotUsed (subId),
  double pcoords[3], double *values, int dim, double *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[3 * 18], sum[3];
  int i, j, k;

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0;
  jI[1] = j1;
  jI[2] = j2;

  this->JacobianInverse (pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (k = 0; k < dim; k++)  //loop over values per vertex
    {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (i = 0; i < 18; i++)  //loop over interp. function derivatives
      {
      sum[0] += functionDerivs[i] * values[dim * i + k];
      sum[1] += functionDerivs[18 + i] * values[dim * i + k];
      sum[2] += functionDerivs[36 + i] * values[dim * i + k];
      }
    for (j = 0; j < 3; j++)  //loop over derivative directions
      {
      derivs[3 * k + j] = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
      }
    }
}

//----------------------------------------------------------------------------
// Compute interpolation functions for the fifteen nodes.
void vtkBiQuadraticQuadraticWedge::InterpolationFunctions (double pcoords[3], double weights[18])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  double x = 2*(pcoords[0]- 0.5);
  double y = 2*(pcoords[1]- 0.5);
  double z = 2*(pcoords[2]- 0.5);

  // corners
  weights[0] =-0.25 * (x + y) * (x + y + 1) * z * (1 - z);
  weights[1] =-0.25 *  x      * (x + 1)     * z * (1 - z);
  weights[2] =-0.25 *      y  * (1 + y)     * z * (1 - z);
  weights[3] = 0.25 * (x + y) * (x + y + 1) * z * (1 + z);
  weights[4] = 0.25 *  x      * (x + 1)     * z * (1 + z);
  weights[5] = 0.25 *      y  * (1 + y)     * z * (1 + z);

  // midsides of quadratic triangles
  weights[6] =  (x + 1)*(x + y) *  0.5 * z * (1 - z);
  weights[7] = -(x + 1)*(y + 1) *  0.5 * z * (1 - z);
  weights[8] =  (y + 1)*(x + y) *  0.5 * z * (1 - z);
  weights[9] = -(x + 1)*(x + y) *  0.5 * z * (1 + z);
  weights[10]=  (x + 1)*(y + 1) *  0.5 * z * (1 + z);
  weights[11]= -(y + 1)*(x + y) *  0.5 * z * (1 + z);

  // midsides of edges between the two triangles
  weights[12] = 0.5 * (x + y) * (x + y + 1) * (1 + z)*(1 - z);
  weights[13] = 0.5 *  x      * (x + 1)     * (1 + z)*(1 - z);
  weights[14] = 0.5 *      y  * (1 + y)     * (1 + z)*(1 - z);

  //Centerpoints of the biquadratic quads
  weights[15] = -(x + 1)*(x + y) * (1 + z)*(1 - z);
  weights[16] =  (x + 1)*(y + 1) * (1 + z)*(1 - z);
  weights[17] = -(y + 1)*(x + y) * (1 + z)*(1 - z);
}

//----------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkBiQuadraticQuadraticWedge::InterpolationDerivs (double pcoords[3], double derivs[54])
{
  //VTK needs parametric coordinates to be between (0,1). Isoparametric
  //shape functions are formulated between (-1,1). Here we do a
  //coordinate system conversion from (0,1) to (-1,1).
  double x = 2*(pcoords[0]- 0.5);
  double y = 2*(pcoords[1]- 0.5);
  double z = 2*(pcoords[2]- 0.5);

  //Derivatives in x-direction
  // corners
  derivs[0] = -0.25 * (2 * x + 2 * y + 1) * z * (1 - z);
  derivs[1] = -0.25 * (2 * x + 1)         * z * (1 - z);
  derivs[2] =  0;
  derivs[3] =  0.25 * (2 * x + 2 * y + 1) * z * (1 + z);
  derivs[4] =  0.25 * (2 * x + 1)         * z * (1 + z);
  derivs[5] =  0;
  // midsides of quadratic triangles
  derivs[6] =  (2 * x + y + 1) *  0.5 * z * (1 - z);
  derivs[7] = -(y + 1)         *  0.5 * z * (1 - z);
  derivs[8] =  (y + 1)         *  0.5 * z * (1 - z);
  derivs[9] = -(2 * x + y + 1) *  0.5 * z * (1 + z);
  derivs[10] = (y + 1)         *  0.5 * z * (1 + z) ;
  derivs[11] =-(y + 1)         *  0.5 * z * (1 + z) ;
  // midsides of edges between the two triangles
  derivs[12] = 0.5 * (2 * x + 2 * y + 1) * (1 + z)*(1 - z);
  derivs[13] = 0.5 * (2 * x + 1)         * (1 + z)*(1 - z);
  derivs[14] = 0;
  //Centerpoints of the biquadratic quads
  derivs[15] = -(2 * x + y + 1) * (1 + z)*(1 - z);
  derivs[16] =  (y + 1)         * (1 + z)*(1 - z);
  derivs[17] = -(y + 1)         * (1 + z)*(1 - z);

  //Derivatives in y-direction
  // corners
  derivs[18] = -0.25 * (2 * y + 2 * x + 1)   * z * (1 - z);
  derivs[19] =  0;
  derivs[20] = -0.25 * (2 * y + 1)           * z * (1 - z);
  derivs[21] =  0.25 * (2 * y + 2 * x + 1)   * z * (1 + z);
  derivs[22] =  0;
  derivs[23] =  0.25 * (2 * y + 1)           * z * (1 + z);
  // midsides of quadratic triangles
  derivs[24] =  (x + 1)         *  0.5 * z * (1 - z);
  derivs[25] = -(x + 1)         *  0.5 * z * (1 - z);
  derivs[26] =  (2 * y + x + 1) *  0.5 * z * (1 - z);
  derivs[27] = -(x + 1)         *  0.5 * z * (1 + z);
  derivs[28] =  (x + 1)         *  0.5 * z * (1 + z);
  derivs[29] = -(2 * y + x + 1) *  0.5 * z * (1 + z);
  // midsides of edges between the two triangles
  derivs[30] = 0.5 * (2 * y + 2 * x + 1) * (1 + z)*(1 - z);
  derivs[31] = 0;
  derivs[32] = 0.5 * (2 * y + 1)         * (1 + z)*(1 - z);
  //Centerpoints of the biquadratic quads
  derivs[33] = -(x + 1)         * (1 + z)*(1 - z);
  derivs[34] =  (x + 1)         * (1 + z)*(1 - z);
  derivs[35] = -(2 * y + x + 1) * (1 + z)*(1 - z);

  //Derivatives in z-direction
  // corners
  derivs[36] = -0.25 * (x + y) * (x + y + 1) * (1 - 2 * z);
  derivs[37] = -0.25 *  x      * (x + 1)     * (1 - 2 * z);
  derivs[38] = -0.25 *      y  * (1 + y)     * (1 - 2 * z);
  derivs[39] =  0.25 * (x + y) * (x + y + 1) * (1 + 2 * z);
  derivs[40] =  0.25 *  x      * (x + 1)     * (1 + 2 * z);
  derivs[41] =  0.25 *      y  * (1 + y)     * (1 + 2 * z);
  // midsides of quadratic triangles
  derivs[42] =  (x + 1)*(x + y) *  0.5 * (1 - 2 * z);
  derivs[43] = -(x + 1)*(y + 1) *  0.5 * (1 - 2 * z);
  derivs[44] =  (y + 1)*(x + y) *  0.5 * (1 - 2 * z);
  derivs[45] = -(x + 1)*(x + y) *  0.5 * (1 + 2 * z);
  derivs[46] =  (x + 1)*(y + 1) *  0.5 * (1 + 2 * z);
  derivs[47] = -(y + 1)*(x + y) *  0.5 * (1 + 2 * z);
  // midsides of edges between the two triangles
  derivs[48] = 0.5 * (x + y) * (x + y + 1) * (-2 * z);
  derivs[49] = 0.5 *  x      * (x + 1)     * (-2 * z);
  derivs[50] = 0.5 *      y  * (1 + y)     * (-2 * z);
  //Centerpoints of the biquadratic quads
  derivs[51] = -(x + 1)*(x + y) * (-2 * z);
  derivs[52] =  (x + 1)*(y + 1) * (-2 * z);
  derivs[53] = -(y + 1)*(x + y) * (-2 * z);

  // we compute derivatives in [-1; 1] but we need them in [ 0; 1]
  for(int i = 0; i < 54; i++)
    derivs[i] *= 2;
}

//----------------------------------------------------------------------------
static double vtkQWedgeCellPCoords[54] = {
  0.0,0.0,0.0, 1.0,0.0,0.0, 0.0,1.0,0.0,
  0.0,0.0,1.0, 1.0,0.0,1.0, 0.0,1.0,1.0,
  0.5,0.0,0.0, 0.5,0.5,0.0, 0.0,0.5,0.0,
  0.5,0.0,1.0, 0.5,0.5,1.0, 0.0,0.5,1.0,
  0.0,0.0,0.5, 1.0,0.0,0.5, 0.0,1.0,0.5,
  0.5,0.0,0.5, 0.5,0.5,0.5, 0.0,0.5,0.5
};

double *vtkBiQuadraticQuadraticWedge::GetParametricCoords()
{
  return vtkQWedgeCellPCoords;
}

//----------------------------------------------------------------------------
void vtkBiQuadraticQuadraticWedge::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf (os, indent.GetNextIndent ());
  os << indent << "TriangleFace:\n";
  this->TriangleFace->PrintSelf (os, indent.GetNextIndent ());
  os << indent << "Face:\n";
  this->Face->PrintSelf (os, indent.GetNextIndent ());
  os << indent << "Wedge:\n";
  this->Wedge->PrintSelf (os, indent.GetNextIndent ());
  os << indent << "Scalars:\n";
  this->Scalars->PrintSelf (os, indent.GetNextIndent ());
}
