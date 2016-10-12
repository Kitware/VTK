/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticLinearWedge.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//Thanks to Soeren Gebbert  who developed this class and
//integrated it into VTK 5.0.

#include "vtkQuadraticLinearWedge.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkWedge.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkQuadraticTriangle.h"
#include "vtkPoints.h"

vtkStandardNewMacro (vtkQuadraticLinearWedge);

//----------------------------------------------------------------------------
// Construct the quadratic linear wedge with 12 points

vtkQuadraticLinearWedge::vtkQuadraticLinearWedge ()
{
  this->Points->SetNumberOfPoints (12);
  this->PointIds->SetNumberOfIds (12);
  for (int i = 0; i < 12; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
  }

  this->QuadEdge = vtkQuadraticEdge::New ();
  this->Edge = vtkLine::New ();
  this->Face = vtkQuadraticLinearQuad::New ();
  this->TriangleFace = vtkQuadraticTriangle::New ();
  this->Wedge = vtkWedge::New ();

  this->Scalars = vtkDoubleArray::New ();
  this->Scalars->SetNumberOfTuples (6);  //num of linear wedge vetices
}

//----------------------------------------------------------------------------
vtkQuadraticLinearWedge::~vtkQuadraticLinearWedge ()
{
  this->QuadEdge->Delete ();
  this->Edge->Delete ();
  this->Face->Delete ();
  this->TriangleFace->Delete ();
  this->Wedge->Delete ();

  this->Scalars->Delete ();
}

//----------------------------------------------------------------------------
// We are using 4 linear wedge
static int LinearWedges[4][6] = {
    {0, 6, 8,  3,  9, 11},
    {6, 7, 8,  9, 10, 11},
    {6, 1, 7,  9,  4, 10},
    {8, 7, 2, 11, 10,  5}
};

// We use 2 quadratic triangles and 3 quadratic-linear quads
static int WedgeFaces[5][6] = {
    {0, 1, 2, 6,  7,  8},   // first quad triangle
    {3, 5, 4, 11, 10, 9},   // second quad triangle
    {1, 0, 3, 4, 6, 9},   // 1. quad-linear quad
    {2, 1, 4, 5, 7,10},   // 2. quad-linear quad
    {0, 2, 5, 3, 8,11}    // 3. quad-linear quad
};

// We have 6 quadratic and 3 linear edges
static int WedgeEdges[9][3] = {
    {0, 1, 6}, {1, 2,  7}, {2, 0,  8}, // quadratic edges
    {3, 4, 9}, {4, 5, 10}, {5, 3, 11},
    {0, 3, 0}, {1, 4,  0}, {2, 5,  0} // linear edges
};

//----------------------------------------------------------------------------
int *vtkQuadraticLinearWedge::GetEdgeArray(int edgeId)
{
  return WedgeEdges[edgeId];
}

//----------------------------------------------------------------------------
int *vtkQuadraticLinearWedge::GetFaceArray(int faceId)
{
  return WedgeFaces[faceId];
}

//----------------------------------------------------------------------------
vtkCell * vtkQuadraticLinearWedge::GetEdge (int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 8 ? 8 : edgeId));

  //We have 6 quadratic edges and 3 linear edges
  if (edgeId < 6)
  {
    for (int i = 0; i < 3; i++)
    {
      this->QuadEdge->PointIds->SetId (i, this->PointIds->GetId (WedgeEdges[edgeId][i]));
      this->QuadEdge->Points->SetPoint (i, this->Points->GetPoint (WedgeEdges[edgeId][i]));
    }
    return this->QuadEdge;
  }
  else
  {
    for (int i = 0; i < 2; i++)
    {
      this->Edge->PointIds->SetId (i, this->PointIds->GetId (WedgeEdges[edgeId][i]));
      this->Edge->Points->SetPoint (i, this->Points->GetPoint (WedgeEdges[edgeId][i]));
    }
    return this->Edge;
  }
}

//----------------------------------------------------------------------------
vtkCell * vtkQuadraticLinearWedge::GetFace (int faceId)
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
    for (int i = 0; i < 6; i++)
    {
      this->Face->PointIds->SetId (i, this->PointIds->GetId (WedgeFaces[faceId][i]));
      this->Face->Points->SetPoint (i, this->Points->GetPoint (WedgeFaces[faceId][i]));
    }
    return this->Face;
  }
}

//----------------------------------------------------------------------------
static const double VTK_DIVERGED = 1.e6;
static const int VTK_WEDGE_MAX_ITERATION = 30;
static const double VTK_WEDGE_CONVERGED = 1.e-03;

int vtkQuadraticLinearWedge::EvaluatePosition (double *x,
  double *closestPoint,
  int &subId, double pcoords[3], double &dist2, double *weights)
{
  int iteration, converged;
  double params[3];
  double fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  double d, pt[3];
  double derivs[3 * 12];

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
    for (i = 0; i < 12; i++)
    {
      this->Points->GetPoint (i, pt);
      for (j = 0; j < 3; j++)
      {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i + 12];
        tcol[j] += pt[j] * derivs[i + 24];
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
      (fabs (pcoords[1]) > VTK_DIVERGED) ||
      (fabs (pcoords[2]) > VTK_DIVERGED))
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
    double pc[3], w[12];
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
void vtkQuadraticLinearWedge::EvaluateLocation (int &vtkNotUsed (subId),
  double pcoords[3], double x[3], double *weights)
{
  double pt[3];

  this->InterpolationFunctions (pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (int i = 0; i < 12; i++)
  {
    this->Points->GetPoint (i, pt);
    for (int j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//----------------------------------------------------------------------------
int vtkQuadraticLinearWedge::CellBoundary (int subId, double pcoords[3], vtkIdList * pts)
{
  return this->Wedge->CellBoundary (subId, pcoords, pts);
}

//----------------------------------------------------------------------------
void vtkQuadraticLinearWedge::Contour (double value,
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
  for (int i=0; i<4; i++) //for each wedge
  {
    for (int j=0; j<6; j++) //for each point of wedge
    {
      this->Wedge->Points->SetPoint(j,this->Points->GetPoint(LinearWedges[i][j]));
      this->Wedge->PointIds->SetId(j,this->PointIds->GetId(LinearWedges[i][j]));
      this->Scalars->SetValue(j,cellScalars->GetTuple1(LinearWedges[i][j]));
    }
      this->Wedge->Contour(value,this->Scalars,locator,verts,lines,polys,
                         inPd,outPd,inCd,cellId,outCd);
  }
}

//----------------------------------------------------------------------------
// Clip this quadratic-linear wedge using scalar value provided. Like contouring,
// except that it cuts the wedge to produce tetrahedra.
void vtkQuadraticLinearWedge::Clip (double value, vtkDataArray *cellScalars,
             vtkIncrementalPointLocator * locator, vtkCellArray * tets,
             vtkPointData * inPd, vtkPointData * outPd,
             vtkCellData * inCd, vtkIdType cellId, vtkCellData * outCd, int insideOut)
{
  //clip each linear wedge separately
  for (int i=0; i<4; i++) //for each wedge
  {
    for (int j=0; j<6; j++) //for each of the six vertices of the wedge
    {
      this->Wedge->Points->SetPoint(j,this->Points->GetPoint(LinearWedges[i][j]));
      this->Wedge->PointIds->SetId(j,this->PointIds->GetId(LinearWedges[i][j]));
      this->Scalars->SetValue(j,cellScalars->GetTuple1(LinearWedges[i][j]));
    }
      this->Wedge->Clip(value,this->Scalars,locator,tets,inPd,outPd,
                    inCd,cellId,outCd,insideOut);
  }

}

//----------------------------------------------------------------------------
// Line-wedge intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkQuadraticLinearWedge::IntersectWithLine (double *p1, double *p2,
              double tol, double &t, double *x, double *pcoords, int &subId)
{
  int intersection = 0;
  double tTemp;
  double pc[3], xTemp[3];
  int faceNum;
  int inter;

  t = VTK_DOUBLE_MAX;
  for (faceNum = 0; faceNum < 5; faceNum++)
  {
    // We have 6 nodes on quad-linear face
    // and 6 on triangle faces
    if (faceNum < 2)
    {
      for (int i = 0; i < 6; i++)
      {
        this->TriangleFace->PointIds->SetId (i, this->PointIds->GetId (WedgeFaces[faceNum][i]));
        this->TriangleFace->Points->SetPoint (i, this->Points->GetPoint (WedgeFaces[faceNum][i]));
      }
      inter = this->TriangleFace->IntersectWithLine (p1, p2, tol, tTemp, xTemp, pc, subId);
    }
    else
    {
      for (int i = 0; i < 6; i++)
      {
        this->Face->Points->SetPoint (i, this->Points->GetPoint (WedgeFaces[faceNum][i]));
      }
      inter = this->Face->IntersectWithLine (p1, p2, tol, tTemp, xTemp, pc, subId);
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
        }
      }
    }
  }
  return intersection;
}

//----------------------------------------------------------------------------
int vtkQuadraticLinearWedge::Triangulate (int vtkNotUsed (index),
  vtkIdList * ptIds, vtkPoints * pts)
{
  pts->Reset ();
  ptIds->Reset ();

  for (int i = 0; i < 4; i++)
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
void vtkQuadraticLinearWedge::JacobianInverse (double pcoords[3],
  double **inverse, double derivs[36])
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

  for (j = 0; j < 12; j++)
  {
    this->Points->GetPoint (j, x);
    for (i = 0; i < 3; i++)
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[12 + j];
      m2[i] += x[i] * derivs[24 + j];
    }
  }

  // now find the inverse
  if (vtkMath::InvertMatrix (m, inverse, 3) == 0)
  {
    vtkErrorMacro (<<"Jacobian inverse not found");
    return;
  }
}

//----------------------------------------------------------------------------
void vtkQuadraticLinearWedge::Derivatives (int vtkNotUsed (subId),
  double pcoords[3], double *values, int dim, double *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[3 * 12], sum[3];
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
    for (i = 0; i < 12; i++)  //loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim * i + k];
      sum[1] += functionDerivs[12 + i] * values[dim * i + k];
      sum[2] += functionDerivs[24 + i] * values[dim * i + k];
    }
    for (j = 0; j < 3; j++)  //loop over derivative directions
    {
      derivs[3 * k + j] = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
    }
  }
}


//----------------------------------------------------------------------------
// Compute interpolation functions for the fifteen nodes.
void vtkQuadraticLinearWedge::InterpolationFunctions (double pcoords[3],
  double weights[12])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  double x = 2*(pcoords[0]- 0.5);
  double y = 2*(pcoords[1]- 0.5);
  double z = 2*(pcoords[2]- 0.5);

  // corners
  weights[0] =(x + y) * 0.5 * (x + y + 1.0) * (1 - z) * 0.5;
  weights[1] = x        * (x + 1.0) * 0.5   * (1 - z) * 0.5;
  weights[2] =     y    * (1.0 + y) * 0.5   * (1 - z) * 0.5;
  weights[3] =(x + y) * 0.5 * (x + y + 1.0) * (1 + z) * 0.5;
  weights[4] = x        * (x + 1.0) * 0.5   * (1 + z) * 0.5;
  weights[5] =     y    * (1.0 + y) * 0.5   * (1 + z) * 0.5;

  // midsides of triangles
  weights[6] = -(x + 1)*(x + y) * (1 - z) * 0.5;
  weights[7] =  (x + 1)*(y + 1) * (1 - z) * 0.5;
  weights[8] = -(y + 1)*(x + y) * (1 - z) * 0.5;
  weights[9] = -(x + 1)*(x + y) * (1 + z) * 0.5;
  weights[10]=  (x + 1)*(y + 1) * (1 + z) * 0.5;
  weights[11]= -(y + 1)*(x + y) * (1 + z) * 0.5;

}

//----------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkQuadraticLinearWedge::InterpolationDerivs (double pcoords[3],
  double derivs[36])
{
  //VTK needs parametric coordinates to be between (0,1). Isoparametric
  //shape functions are formulated between (-1,1). Here we do a
  //coordinate system conversion from (0,1) to (-1,1).
  double x = 2*(pcoords[0]- 0.5);
  double y = 2*(pcoords[1]- 0.5);
  double z = 2*(pcoords[2]- 0.5);

  //Derivatives in x-direction
  // corners
  derivs[0] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 - z) * 0.5;
  derivs[1] = (1.0 + 2.0 * x) *           0.5 * (1.0 - z) * 0.5;
  derivs[2] = 0;
  derivs[3] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 + z) * 0.5;
  derivs[4] = (1.0 + 2.0 * x) *           0.5 * (1.0 + z) * 0.5;
  derivs[5] = 0;

  // midsides of triangles
  derivs[6] = -(2.0 * x + y + 1.0) * (1.0 - z) * 0.5;
  derivs[7] =  (y + 1.0) *           (1.0 - z) * 0.5;
  derivs[8] = -(y + 1.0) *           (1.0 - z) * 0.5;
  derivs[9] = -(2.0 * x + y + 1.0) * (1.0 + z) * 0.5;
  derivs[10] = (y + 1.0) *           (1.0 + z) * 0.5;
  derivs[11] =-(y + 1.0) *           (1.0 + z) * 0.5;

  //Derivatives in y-direction
  // corners
  derivs[12] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 - z) * 0.5;
  derivs[13] = 0;
  derivs[14] = (1.0 + 2.0 * y) *           0.5 * (1.0 - z) * 0.5;
  derivs[15] = (2.0 * x + 2.0 * y + 1.0) * 0.5 * (1.0 + z) * 0.5;
  derivs[16] = 0;
  derivs[17] = (1.0 + 2.0 * y) *           0.5 * (1.0 + z) * 0.5;

  // midsides of triangles
  derivs[18] = -(x + 1.0) *           (1.0 - z) * 0.5;
  derivs[19] =  (x + 1.0) *           (1.0 - z) * 0.5;
  derivs[20] = -(x + 2.0 * y + 1.0) * (1.0 - z) * 0.5;
  derivs[21] = -(x + 1.0) *           (1.0 + z) * 0.5;
  derivs[22] =  (x + 1.0) *           (1.0 + z) * 0.5;
  derivs[23] = -(x + 2.0 * y + 1.0) * (1.0 + z) * 0.5;

  //Derivatives in z-direction
  // corners
  derivs[24] =(x + y) * 0.5 * (x + y + 1.0) * -0.5;
  derivs[25] = x        * (x + 1.0) * 0.5   * -0.5;
  derivs[26] =     y    * (1.0 + y) * 0.5   * -0.5;
  derivs[27] =(x + y) * 0.5 * (x + y + 1.0) *  0.5;
  derivs[28] = x        * (x + 1.0) * 0.5   *  0.5;
  derivs[29] =     y    * (1.0 + y) * 0.5   *  0.5;

  // midsides of triangles
  derivs[30] = -(x + 1.0) * (x + y)   * -0.5;
  derivs[31] =  (x + 1.0) * (y + 1.0) * -0.5;
  derivs[32] = -(y + 1.0) * (x + y)   * -0.5;
  derivs[33] = -(x + 1.0) * (x + y)   *  0.5;
  derivs[34] =  (x + 1.0) * (y + 1.0) *  0.5;
  derivs[35] = -(y + 1.0) * (x + y)   *  0.5;

  // we compute derivatives in in [-1; 1] but we need them in [ 0; 1]
  for(int i = 0; i < 36; i++)
    derivs[i] *= 2;
}

//----------------------------------------------------------------------------
static double vtkQWedgeCellPCoords[36] = {
  0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
  0.0, 0.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0, 1.0,
  0.5, 0.0, 0.0, 0.5, 0.5, 0.0, 0.0, 0.5, 0.0,
  0.5, 0.0, 1.0, 0.5, 0.5, 1.0, 0.0, 0.5, 1.0
};

double * vtkQuadraticLinearWedge::GetParametricCoords ()
{
  return vtkQWedgeCellPCoords;
}

//----------------------------------------------------------------------------
void vtkQuadraticLinearWedge::PrintSelf (ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf (os, indent);

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
