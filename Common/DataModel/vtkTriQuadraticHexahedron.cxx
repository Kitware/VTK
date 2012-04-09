/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriQuadraticHexahedron.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//Thanks to Soeren Gebbert  who developed this class and
//integrated it into VTK 5.0.

#include "vtkTriQuadraticHexahedron.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkMath.h"
#include "vtkQuadraticEdge.h"
#include "vtkBiQuadraticQuad.h"
#include "vtkPoints.h"

vtkStandardNewMacro (vtkTriQuadraticHexahedron);

//----------------------------------------------------------------------------
// Construct the triquadhex with 27 nodes
vtkTriQuadraticHexahedron::vtkTriQuadraticHexahedron ()
{
  this->Points->SetNumberOfPoints (27);
  this->PointIds->SetNumberOfIds (27);
  for (int i = 0; i < 27; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }

  this->Edge = vtkQuadraticEdge::New ();
  this->Face = vtkBiQuadraticQuad::New ();
  this->Hex = vtkHexahedron::New ();

  this->Scalars = vtkDoubleArray::New ();
  this->Scalars->SetNumberOfTuples (8); // vertices of a linear hexahedron

}

//----------------------------------------------------------------------------
vtkTriQuadraticHexahedron::~vtkTriQuadraticHexahedron ()
{
  this->Edge->Delete ();
  this->Face->Delete ();
  this->Hex->Delete ();

  this->Scalars->Delete ();
}

//----------------------------------------------------------------------------
static int LinearHexs[8][8] = {
    { 0,  8, 24, 11, 16, 22, 26, 20},
    { 8,  1,  9, 24, 22, 17, 21, 26},
    {11, 24, 10,  3, 20, 26, 23, 19},
    {24,  9,  2, 10, 26, 21, 18, 23},
    {16, 22, 26, 20,  4, 12, 25, 15},
    {22, 17, 21, 26, 12,  5, 13, 25},
    {20, 26, 23, 19, 15, 25, 14,  7},
    {26, 21, 18, 23, 25, 13,  6, 14} };

static int HexFaces[6][9] = {
    {0, 4, 7, 3, 16, 15, 19, 11, 20},
    {1, 2, 6, 5,  9, 18, 13, 17, 21},
    {0, 1, 5, 4,  8, 17, 12, 16, 22},
    {3, 7, 6, 2, 19, 14, 18, 10, 23},
    {0, 3, 2, 1, 11, 10,  9,  8, 24},
    {4, 5, 6, 7, 12, 13, 14, 15, 25} };

static int HexEdges[12][3] = {
    {0, 1,  8}, {1, 2,  9}, {3, 2, 10}, {0, 3, 11},
    {4, 5, 12}, {5, 6, 13}, {7, 6, 14}, {4, 7, 15},
    {0, 4, 16}, {1, 5, 17}, {3, 7, 19}, {2, 6, 18}
};

//----------------------------------------------------------------------------
int *vtkTriQuadraticHexahedron::GetEdgeArray(int edgeId)
{
  return HexEdges[edgeId];
}
//----------------------------------------------------------------------------
int *vtkTriQuadraticHexahedron::GetFaceArray(int faceId)
{
  return HexFaces[faceId];
}

//----------------------------------------------------------------------------
vtkCell * vtkTriQuadraticHexahedron::GetEdge (int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 11 ? 11 : edgeId));

  for (int i = 0; i < 3; i++)
    {
    this->Edge->PointIds->SetId (i, this->PointIds->GetId (HexEdges[edgeId][i]));
    this->Edge->Points->SetPoint (i, this->Points->GetPoint (HexEdges[edgeId][i]));
    }

  return this->Edge;
}

//----------------------------------------------------------------------------
vtkCell * vtkTriQuadraticHexahedron::GetFace (int faceId)
{
  faceId = (faceId < 0 ? 0 : (faceId > 5 ? 5 : faceId));

  for (int i = 0; i < 9; i++)
    {
    this->Face->PointIds->SetId (i, this->PointIds->GetId (HexFaces[faceId][i]));
    this->Face->Points->SetPoint (i, this->Points->GetPoint (HexFaces[faceId][i]));
    }

  return this->Face;
}

//----------------------------------------------------------------------------
static const double VTK_DIVERGED = 1.e6;
static const int VTK_HEX_MAX_ITERATION = 10;
static const double VTK_HEX_CONVERGED = 1.e-03;

int vtkTriQuadraticHexahedron::EvaluatePosition (double *x,
               double *closestPoint,
               int &subId, double pcoords[3], double &dist2, double *weights)
{
  int iteration, converged;
  double params[3];
  double fcol[3], rcol[3], scol[3], tcol[3];
  int i, j;
  double d = 0.0;
  double pt[3];
  double derivs[81];
  double hexweights[8];

  //  set initial position for Newton's method
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2] = 0.5;
  subId = 0;

  // Use a tri-linear hexahederon to get good starting values
  vtkHexahedron *hex = vtkHexahedron::New();
  for(i = 0; i < 8; i++)
    hex->GetPoints()->SetPoint(i, this->Points->GetPoint(i));

  hex->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, hexweights);
  hex->Delete();

  params[0]  = pcoords[0];
  params[1]  = pcoords[1];
  params[2]  = pcoords[2];

  //  enter iteration loop
  for (iteration = converged = 0; !converged && (iteration < VTK_HEX_MAX_ITERATION); iteration++)
    {
    //  calculate element interpolation functions and derivatives
    this->InterpolationFunctions (pcoords, weights);
    this->InterpolationDerivs (pcoords, derivs);

    //  calculate newton functions
    for (i = 0; i < 3; i++)
      {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
      }
    for (i = 0; i < 27; i++)
      {
      this->Points->GetPoint (i, pt);
      for (j = 0; j < 3; j++)
        {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i + 27];
        tcol[j] += pt[j] * derivs[i + 54];
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
      vtkErrorMacro (<<"Determinant incorrect, iteration " << iteration);
      return -1;
      }

    pcoords[0] = params[0] - 0.5 * vtkMath::Determinant3x3 (fcol, scol, tcol) / d;
    pcoords[1] = params[1] - 0.5 * vtkMath::Determinant3x3 (rcol, fcol, tcol) / d;
    pcoords[2] = params[2] - 0.5 * vtkMath::Determinant3x3 (rcol, scol, fcol) / d;

    //  check for convergence
    if (((fabs (pcoords[0] - params[0])) < VTK_HEX_CONVERGED) &&
      ((fabs (pcoords[1] - params[1])) < VTK_HEX_CONVERGED) &&
      ((fabs (pcoords[2] - params[2])) < VTK_HEX_CONVERGED))
      {
      converged = 1;
      }

    // Test for bad divergence (S.Hirschberg 11.12.2001)
    else if ((fabs (pcoords[0]) > VTK_DIVERGED) ||
      (fabs (pcoords[1]) > VTK_DIVERGED) || (fabs (pcoords[2]) > VTK_DIVERGED))
      {
      vtkErrorMacro (<<"Newton did not converged, iteration " << iteration << " det " << d);
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
    vtkErrorMacro (<<"Newton did not converged, iteration " << iteration << " det " << d);
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
      dist2 = 0.0;    //inside hexahedron
      }
    return 1;
    }
  else
    {
    double pc[3], w[27];
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
void vtkTriQuadraticHexahedron::EvaluateLocation (int &vtkNotUsed (subId),
  double pcoords[3], double x[3], double *weights)
{
  int i, j;
  double pt[3];

  this->InterpolationFunctions (pcoords, weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i = 0; i < 27; i++)
    {
    this->Points->GetPoint (i, pt);
    for (j = 0; j < 3; j++)
      {
      x[j] += pt[j] * weights[i];
      }
    }
}

//----------------------------------------------------------------------------
int
vtkTriQuadraticHexahedron::CellBoundary (int subId, double pcoords[3], vtkIdList * pts)
{
  return this->Hex->CellBoundary (subId, pcoords, pts);
}

//----------------------------------------------------------------------------
void vtkTriQuadraticHexahedron::Contour (double value,
            vtkDataArray * cellScalars,
            vtkIncrementalPointLocator * locator,
            vtkCellArray * verts,
            vtkCellArray * lines,
            vtkCellArray * polys,
            vtkPointData * inPd,
            vtkPointData * outPd,
            vtkCellData * inCd, vtkIdType cellId, vtkCellData * outCd)
{
  //contour each linear hex separately
  for (int i = 0; i < 8; i++)
    {
    for (int j = 0; j < 8; j++)
      {
      this->Hex->Points->SetPoint (j, this->Points->GetPoint (LinearHexs[i][j]));
      this->Hex->PointIds->SetId (j, this->PointIds->GetId(LinearHexs[i][j]));
      this->Scalars->SetValue (j, cellScalars->GetTuple1 (LinearHexs[i][j]));
      }
    this->Hex->Contour (value, this->Scalars, locator, verts, lines, polys,
      inPd, outPd, inCd, cellId, outCd);
    }
}

//----------------------------------------------------------------------------
// Clip this triquadratic hex using scalar value provided. Like contouring,
// except that it cuts the hex to produce tetrahedra.
void vtkTriQuadraticHexahedron::Clip (double value,
         vtkDataArray * cellScalars,
         vtkIncrementalPointLocator * locator,
         vtkCellArray * tets,
         vtkPointData * inPd,
         vtkPointData * outPd,
         vtkCellData * inCd, vtkIdType cellId,
         vtkCellData * outCd, int insideOut)
{
  //clip each linear hex separately
  for (int i = 0; i < 8; i++)
    {
    for (int j = 0; j < 8; j++)
      {
      this->Hex->Points->SetPoint (j, this->Points->GetPoint (LinearHexs[i][j]));
      this->Hex->PointIds->SetId (j, this->PointIds->GetId(LinearHexs[i][j]));
      this->Scalars->SetValue (j, cellScalars->GetTuple1 (LinearHexs[i][j]));
      }
    this->Hex->Clip (value, this->Scalars, locator, tets, inPd, outPd, inCd, cellId, outCd, insideOut);
    }
}

//----------------------------------------------------------------------------
// Line-hex intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkTriQuadraticHexahedron::IntersectWithLine (double *p1, double *p2,
                double tol, double &t, double *x, double *pcoords, int &subId)
{
  int intersection = 0;
  double tTemp;
  double pc[3], xTemp[3];
  int faceNum;

  t = VTK_DOUBLE_MAX;
  for (faceNum = 0; faceNum < 6; faceNum++)
    {
    for (int i = 0; i < 9; i++)
      {
      this->Face->PointIds->SetId(i,this->PointIds->GetId(HexFaces[faceNum][i]));
      this->Face->Points->SetPoint (i, this->Points->GetPoint (HexFaces[faceNum][i]));
      }

    if (this->Face->IntersectWithLine (p1, p2, tol, tTemp, xTemp, pc, subId))
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
int vtkTriQuadraticHexahedron::Triangulate (int vtkNotUsed (index), vtkIdList * ptIds, vtkPoints * pts)
{
  pts->Reset ();
  ptIds->Reset ();

  ptIds->InsertId (0, this->PointIds->GetId (0));
  pts->InsertPoint (0, this->Points->GetPoint (0));

  ptIds->InsertId (1, this->PointIds->GetId (1));
  pts->InsertPoint (1, this->Points->GetPoint (1));

  return 1;
}

//----------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void
vtkTriQuadraticHexahedron::JacobianInverse (double pcoords[3], double **inverse, double derivs[81])
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

  for (j = 0; j < 27; j++)
    {
    this->Points->GetPoint (j, x);
    for (i = 0; i < 3; i++)
      {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[27 + j];
      m2[i] += x[i] * derivs[54 + j];
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
void vtkTriQuadraticHexahedron::Derivatives (int vtkNotUsed (subId),
          double pcoords[3], double *values, int dim, double *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[81], sum[3];
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
    for (i = 0; i < 27; i++)  //loop over interp. function derivatives
      {
      sum[0] += functionDerivs[i] * values[dim * i + k];
      sum[1] += functionDerivs[27 + i] * values[dim * i + k];
      sum[2] += functionDerivs[54 + i] * values[dim * i + k];
      }
    for (j = 0; j < 3; j++)  //loop over derivative directions
      {
      derivs[3 * k + j] = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
      }
    }
}



//----------------------------------------------------------------------------
// Compute interpolation functions for the 27 nodes.
void vtkTriQuadraticHexahedron::InterpolationFunctions (double pcoords[3], double weights[27])
{
  //VTK needs parametric coordinates to be between (0,1). Isoparametric
  //shape functions are formulated between (-1,1). Here we do a
  //coordinate system conversion from (0,1) to (-1,1).
  double r = 2.0 * (pcoords[0] - 0.5);
  double s = 2.0 * (pcoords[1] - 0.5);
  double t = 2.0 * (pcoords[2] - 0.5);

  double g1r = -0.5 * r * (1 - r);
  double g1s = -0.5 * s * (1 - s);
  double g1t = -0.5 * t * (1 - t);

  double g2r = (1 + r)*(1 - r);
  double g2s = (1 + s)*(1 - s);
  double g2t = (1 + t)*(1 - t);

  double g3r = 0.5 * r * (1 + r);
  double g3s = 0.5 * s * (1 + s);
  double g3t = 0.5 * t * (1 + t);


  //The eight corner points
  weights[0] = g1r * g1s * g1t;
  weights[1] = g3r * g1s * g1t;
  weights[2] = g3r * g3s * g1t;
  weights[3] = g1r * g3s * g1t;
  weights[4] = g1r * g1s * g3t;
  weights[5] = g3r * g1s * g3t;
  weights[6] = g3r * g3s * g3t;
  weights[7] = g1r * g3s * g3t;

  //The mid-edge nodes
  weights[8] =  g2r * g1s * g1t;
  weights[9] =  g3r * g2s * g1t;
  weights[10] = g2r * g3s * g1t;
  weights[11] = g1r * g2s * g1t;
  weights[12] = g2r * g1s * g3t;
  weights[13] = g3r * g2s * g3t;
  weights[14] = g2r * g3s * g3t;
  weights[15] = g1r * g2s * g3t;
  weights[16] = g1r * g1s * g2t;
  weights[17] = g3r * g1s * g2t;
  weights[18] = g3r * g3s * g2t;
  weights[19] = g1r * g3s * g2t;

  //face center nodes
  weights[22] = g2r * g1s * g2t;
  weights[21] = g3r * g2s * g2t;
  weights[23] = g2r * g3s * g2t;
  weights[20] = g1r * g2s * g2t;
  weights[24] = g2r * g2s * g1t;
  weights[25] = g2r * g2s * g3t;

  //Cell center node
  weights[26] = g2r * g2s * g2t;
}

//----------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkTriQuadraticHexahedron::InterpolationDerivs (double pcoords[3],
  double derivs[81])
{
  //VTK needs parametric coordinates to be between (0,1). Isoparametric
  //shape functions are formulated between (-1,1). Here we do a
  //coordinate system conversion from (0,1) to (-1,1).
  double r = 2.0 * (pcoords[0] - 0.5);
  double s = 2.0 * (pcoords[1] - 0.5);
  double t = 2.0 * (pcoords[2] - 0.5);

  double g1r = -0.5 * r * (1 - r);
  double g1s = -0.5 * s * (1 - s);
  double g1t = -0.5 * t * (1 - t);

  double g2r = (1 + r)*(1 - r);
  double g2s = (1 + s)*(1 - s);
  double g2t = (1 + t)*(1 - t);

  double g3r = 0.5 * r * (1 + r);
  double g3s = 0.5 * s * (1 + s);
  double g3t = 0.5 * t * (1 + t);

  double g1r_r = r - 0.5;
  double g1s_s = s - 0.5;
  double g1t_t = t - 0.5;

  double g2r_r = -2*r;
  double g2s_s = -2*s;
  double g2t_t = -2*t;

  double g3r_r = r + 0.5;
  double g3s_s = s + 0.5;
  double g3t_t = t + 0.5;


  //r-derivatives
  derivs[0] = g1r_r * g1s * g1t;
  derivs[1] = g3r_r * g1s * g1t;
  derivs[2] = g3r_r * g3s * g1t;
  derivs[3] = g1r_r * g3s * g1t;
  derivs[4] = g1r_r * g1s * g3t;
  derivs[5] = g3r_r * g1s * g3t;
  derivs[6] = g3r_r * g3s * g3t;
  derivs[7] = g1r_r * g3s * g3t;
  derivs[8] = g2r_r * g1s * g1t;
  derivs[9] = g3r_r * g2s * g1t;
  derivs[10]= g2r_r * g3s * g1t;
  derivs[11]= g1r_r * g2s * g1t;
  derivs[12]= g2r_r * g1s * g3t;
  derivs[13]= g3r_r * g2s * g3t;
  derivs[14]= g2r_r * g3s * g3t;
  derivs[15]= g1r_r * g2s * g3t;
  derivs[16]= g1r_r * g1s * g2t;
  derivs[17]= g3r_r * g1s * g2t;
  derivs[18]= g3r_r * g3s * g2t;
  derivs[19]= g1r_r * g3s * g2t;
  derivs[20]= g1r_r * g2s * g2t;
  derivs[21]= g3r_r * g2s * g2t;
  derivs[22]= g2r_r * g1s * g2t;
  derivs[23]= g2r_r * g3s * g2t;
  derivs[24]= g2r_r * g2s * g1t;
  derivs[25]= g2r_r * g2s * g3t;
  derivs[26]= g2r_r * g2s * g2t;

  //s-derivatives
  derivs[27] = g1r * g1s_s * g1t;
  derivs[28] = g3r * g1s_s * g1t;
  derivs[29] = g3r * g3s_s * g1t;
  derivs[30] = g1r * g3s_s * g1t;
  derivs[31] = g1r * g1s_s * g3t;
  derivs[32] = g3r * g1s_s * g3t;
  derivs[33] = g3r * g3s_s * g3t;
  derivs[34] = g1r * g3s_s * g3t;
  derivs[35] = g2r * g1s_s * g1t;
  derivs[36] = g3r * g2s_s * g1t;
  derivs[37] = g2r * g3s_s * g1t;
  derivs[38] = g1r * g2s_s * g1t;
  derivs[39] = g2r * g1s_s * g3t;
  derivs[40] = g3r * g2s_s * g3t;
  derivs[41] = g2r * g3s_s * g3t;
  derivs[42] = g1r * g2s_s * g3t;
  derivs[43] = g1r * g1s_s * g2t;
  derivs[44] = g3r * g1s_s * g2t;
  derivs[45] = g3r * g3s_s * g2t;
  derivs[46] = g1r * g3s_s * g2t;
  derivs[47] = g1r * g2s_s * g2t;
  derivs[48] = g3r * g2s_s * g2t;
  derivs[49] = g2r * g1s_s * g2t;
  derivs[50] = g2r * g3s_s * g2t;
  derivs[51] = g2r * g2s_s * g1t;
  derivs[52] = g2r * g2s_s * g3t;
  derivs[53] = g2r * g2s_s * g2t;

  //t-derivatives
  derivs[54] = g1r * g1s * g1t_t;
  derivs[55] = g3r * g1s * g1t_t;
  derivs[56] = g3r * g3s * g1t_t;
  derivs[57] = g1r * g3s * g1t_t;
  derivs[58] = g1r * g1s * g3t_t;
  derivs[59] = g3r * g1s * g3t_t;
  derivs[60] = g3r * g3s * g3t_t;
  derivs[61] = g1r * g3s * g3t_t;
  derivs[62] = g2r * g1s * g1t_t;
  derivs[63] = g3r * g2s * g1t_t;
  derivs[64] = g2r * g3s * g1t_t;
  derivs[65] = g1r * g2s * g1t_t;
  derivs[66] = g2r * g1s * g3t_t;
  derivs[67] = g3r * g2s * g3t_t;
  derivs[68] = g2r * g3s * g3t_t;
  derivs[69] = g1r * g2s * g3t_t;
  derivs[70] = g1r * g1s * g2t_t;
  derivs[71] = g3r * g1s * g2t_t;
  derivs[72] = g3r * g3s * g2t_t;
  derivs[73] = g1r * g3s * g2t_t;
  derivs[74] = g1r * g2s * g2t_t;
  derivs[75] = g3r * g2s * g2t_t;
  derivs[76] = g2r * g1s * g2t_t;
  derivs[77] = g2r * g3s * g2t_t;
  derivs[78] = g2r * g2s * g1t_t;
  derivs[79] = g2r * g2s * g3t_t;
  derivs[80] = g2r * g2s * g2t_t;

  // we compute derivatives in in [-1; 1] but we need them in [ 0; 1]  
  for(int i = 0; i < 81; i++)
    derivs[i] *= 2;

}

//----------------------------------------------------------------------------
static double vtkQHexCellPCoords[81] = {
  0.0, 0.0, 0.0,
  1.0, 0.0, 0.0,
  1.0, 1.0, 0.0,
  0.0, 1.0, 0.0,
  0.0, 0.0, 1.0,
  1.0, 0.0, 1.0,
  1.0, 1.0, 1.0,
  0.0, 1.0, 1.0,
  0.5, 0.0, 0.0,
  1.0, 0.5, 0.0,
  0.5, 1.0, 0.0,
  0.0, 0.5, 0.0,
  0.5, 0.0, 1.0,
  1.0, 0.5, 1.0,
  0.5, 1.0, 1.0,
  0.0, 0.5, 1.0,
  0.0, 0.0, 0.5,
  1.0, 0.0, 0.5,
  1.0, 1.0, 0.5,
  0.0, 1.0, 0.5,
  0.0, 0.5, 0.5, // 20
  1.0, 0.5, 0.5, // 21
  0.5, 0.0, 0.5, // 22
  0.5, 1.0, 0.5, // 23
  0.5, 0.5, 0.0, // 24
  0.5, 0.5, 1.0, // 25
  0.5, 0.5, 0.5  // 26
};

double *vtkTriQuadraticHexahedron::GetParametricCoords ()
{
  return vtkQHexCellPCoords;
}

//----------------------------------------------------------------------------
void vtkTriQuadraticHexahedron::PrintSelf (ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf (os, indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf (os, indent.GetNextIndent ());
  os << indent << "Face:\n";
  this->Face->PrintSelf (os, indent.GetNextIndent ());
  os << indent << "Hex:\n";
  this->Hex->PrintSelf (os, indent.GetNextIndent ());
  os << indent << "Scalars:\n";
  this->Scalars->PrintSelf (os, indent.GetNextIndent ());
}
