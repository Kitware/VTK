/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiQuadraticQuad.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//Thanks to Soeren Gebbert  who developed this class and
//integrated it into VTK 5.0.

#include "vtkBiQuadraticQuad.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkQuad.h"
#include "vtkQuadraticEdge.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkBiQuadraticQuad);

//----------------------------------------------------------------------------
// Construct the quad with nine points.
vtkBiQuadraticQuad::vtkBiQuadraticQuad()
{
  this->Edge = vtkQuadraticEdge::New();
  this->Quad = vtkQuad::New();
  this->Points->SetNumberOfPoints(9);
  this->PointIds->SetNumberOfIds(9);
  for (int i = 0; i < 9; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
  }
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(4);
}

//----------------------------------------------------------------------------
vtkBiQuadraticQuad::~vtkBiQuadraticQuad()
{
  this->Edge->Delete();
  this->Quad->Delete();

  this->Scalars->Delete();
}

//----------------------------------------------------------------------------
vtkCell *vtkBiQuadraticQuad::GetEdge(int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 3 ? 3 : edgeId));
  int p = (edgeId + 1) % 4;

  // load point id's
  this->Edge->PointIds->SetId (0, this->PointIds->GetId(edgeId));
  this->Edge->PointIds->SetId (1, this->PointIds->GetId(p));
  this->Edge->PointIds->SetId (2, this->PointIds->GetId(edgeId + 4));

  // load coordinates
  this->Edge->Points->SetPoint (0, this->Points->GetPoint(edgeId));
  this->Edge->Points->SetPoint (1, this->Points->GetPoint(p));
  this->Edge->Points->SetPoint (2, this->Points->GetPoint(edgeId + 4));

  return this->Edge;
}

//----------------------------------------------------------------------------
static int LinearQuads[4][4] = { {0, 4, 8, 7}, {4, 1, 5, 8},
                                 {8, 5, 2, 6}, {7, 8, 6, 3} };

//----------------------------------------------------------------------------
int vtkBiQuadraticQuad::EvaluatePosition (double *x,
                                          double *closestPoint,
                                          int &subId, double pcoords[3],
                                          double &minDist2, double *weights)
{
  double pc[3], dist2;
  int ignoreId, i, returnStatus = 0, status;
  double tempWeights[4];
  double closest[3];

  //four linear quads are used
  for (minDist2 = VTK_DOUBLE_MAX, i = 0; i < 4; i++)
  {
    this->Quad->Points->SetPoint (0,
      this->Points->GetPoint (LinearQuads[i][0]));
    this->Quad->Points->SetPoint (1,
      this->Points->GetPoint (LinearQuads[i][1]));
    this->Quad->Points->SetPoint (2,
      this->Points->GetPoint (LinearQuads[i][2]));
    this->Quad->Points->SetPoint (3,
      this->Points->GetPoint (LinearQuads[i][3]));

    status = this->Quad->EvaluatePosition (x, closest, ignoreId, pc, dist2,
      tempWeights);
    if (status != -1 && dist2 < minDist2)
    {
      returnStatus = status;
      minDist2 = dist2;
      subId = i;
      pcoords[0] = pc[0];
      pcoords[1] = pc[1];
    }
  }

  // adjust parametric coordinates
  if (returnStatus != -1)
  {
    if (subId == 0)
    {
      pcoords[0] /= 2.0;
      pcoords[1] /= 2.0;
    }
    else if (subId == 1)
    {
      pcoords[0] = 0.5 + (pcoords[0] / 2.0);
      pcoords[1] /= 2.0;
    }
    else if (subId == 2)
    {
      pcoords[0] = 0.5 + (pcoords[0] / 2.0);
      pcoords[1] = 0.5 + (pcoords[1] / 2.0);
    }
    else
    {
      pcoords[0] /= 2.0;
      pcoords[1] = 0.5 + (pcoords[1] / 2.0);
    }
    pcoords[2] = 0.0;
    if(closestPoint!=0)
    {
      // Compute both closestPoint and weights
      this->EvaluateLocation(subId,pcoords,closestPoint,weights);
    }
    else
    {
      // Compute weigths only
      this->InterpolationFunctionsPrivate(pcoords,weights);
    }
  }

  return returnStatus;
}

//----------------------------------------------------------------------------
void vtkBiQuadraticQuad::EvaluateLocation (int& vtkNotUsed(subId),
                                           double pcoords[3],
                                           double x[3], double *weights)
{
  int i, j;
  double pt[3];

  this->InterpolationFunctionsPrivate(pcoords,weights);

  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<9; i++)
  {
    this->Points->GetPoint(i, pt);
    for (j=0; j<3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//----------------------------------------------------------------------------
int vtkBiQuadraticQuad::CellBoundary (int subId, double pcoords[3], vtkIdList * pts)
{
  return this->Quad->CellBoundary (subId, pcoords, pts);
}


//----------------------------------------------------------------------------
void
vtkBiQuadraticQuad::Contour (double value,
           vtkDataArray *cellScalars,
           vtkIncrementalPointLocator * locator,
           vtkCellArray * verts,
           vtkCellArray * lines,
           vtkCellArray * polys,
           vtkPointData * inPd,
           vtkPointData * outPd, vtkCellData * inCd, vtkIdType cellId, vtkCellData * outCd)
{
  //contour each linear quad separately
  for (int i=0; i<4; i++)
  {
    for (int j=0; j<4; j++)
    {
      this->Quad->Points->SetPoint(j,this->Points->GetPoint(LinearQuads[i][j]));
      this->Quad->PointIds->SetId(j,this->PointIds->GetId(LinearQuads[i][j]));
      this->Scalars->SetValue(j,cellScalars->GetTuple1(LinearQuads[i][j]));
    }

    this->Quad->Contour(value,this->Scalars,locator,verts,lines,polys,
                        inPd,outPd,inCd,cellId,outCd);
  }
}

//----------------------------------------------------------------------------
// Clip this quadratic quad using scalar value provided. Like contouring,
// except that it cuts the quad to produce other quads and triangles.
void
vtkBiQuadraticQuad::Clip (double value, vtkDataArray * cellScalars,
        vtkIncrementalPointLocator * locator, vtkCellArray * polys,
        vtkPointData * inPd, vtkPointData * outPd,
        vtkCellData * inCd, vtkIdType cellId, vtkCellData * outCd, int insideOut)
{
  //contour each linear quad separately
  for (int i=0; i<4; i++)
  {
    for ( int j=0; j<4; j++) //for each of the four vertices of the linear quad
    {
      this->Quad->Points->SetPoint(j,this->Points->GetPoint(LinearQuads[i][j]));
      this->Quad->PointIds->SetId(j,this->PointIds->GetId(LinearQuads[i][j]));
      this->Scalars->SetValue(j,cellScalars->GetTuple1(LinearQuads[i][j]));
    }

    this->Quad->Clip(value,this->Scalars,locator,polys,inPd,
                     outPd,inCd,cellId,outCd,insideOut);
  }
}

//----------------------------------------------------------------------------
// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int
vtkBiQuadraticQuad::IntersectWithLine (double *p1,
               double *p2, double tol, double &t, double *x, double *pcoords, int &subId)
{
  int subTest, i;
  subId = 0;

  //intersect the four linear quads
  for (i = 0; i < 4; i++)
  {
    this->Quad->Points->SetPoint (0,
      this->Points->GetPoint(LinearQuads[i][0]));
    this->Quad->Points->SetPoint (1,
      this->Points->GetPoint(LinearQuads[i][1]));
    this->Quad->Points->SetPoint (2,
      this->Points->GetPoint(LinearQuads[i][2]));
    this->Quad->Points->SetPoint (3,
      this->Points->GetPoint(LinearQuads[i][3]));

    if (this->Quad->IntersectWithLine (p1, p2, tol, t, x, pcoords, subTest))
    {
      return 1;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkBiQuadraticQuad::Triangulate (int vtkNotUsed(index),
                                     vtkIdList *ptIds,
                                     vtkPoints *pts)
{
  pts->SetNumberOfPoints(24);
  ptIds->SetNumberOfIds(24);

  // First the corner vertices
  ptIds->SetId(0,this->PointIds->GetId(0));
  ptIds->SetId(1,this->PointIds->GetId(4));
  ptIds->SetId(2,this->PointIds->GetId(7));
  pts->SetPoint(0,this->Points->GetPoint(0));
  pts->SetPoint(1,this->Points->GetPoint(4));
  pts->SetPoint(2,this->Points->GetPoint(7));

  ptIds->SetId(3,this->PointIds->GetId(4));
  ptIds->SetId(4,this->PointIds->GetId(1));
  ptIds->SetId(5,this->PointIds->GetId(5));
  pts->SetPoint(3,this->Points->GetPoint(4));
  pts->SetPoint(4,this->Points->GetPoint(1));
  pts->SetPoint(5,this->Points->GetPoint(5));

  ptIds->SetId(6,this->PointIds->GetId(5));
  ptIds->SetId(7,this->PointIds->GetId(2));
  ptIds->SetId(8,this->PointIds->GetId(6));
  pts->SetPoint(6,this->Points->GetPoint(5));
  pts->SetPoint(7,this->Points->GetPoint(2));
  pts->SetPoint(8,this->Points->GetPoint(6));

  ptIds->SetId(9,this->PointIds->GetId(6));
  ptIds->SetId(10,this->PointIds->GetId(3));
  ptIds->SetId(11,this->PointIds->GetId(7));
  pts->SetPoint(9,this->Points->GetPoint(6));
  pts->SetPoint(10,this->Points->GetPoint(3));
  pts->SetPoint(11,this->Points->GetPoint(7));

  //Now the triangles in the middle
  ptIds->SetId(12,this->PointIds->GetId(4));
  ptIds->SetId(13,this->PointIds->GetId(8));
  ptIds->SetId(14,this->PointIds->GetId(7));
  pts->SetPoint(12,this->Points->GetPoint(4));
  pts->SetPoint(13,this->Points->GetPoint(8));
  pts->SetPoint(14,this->Points->GetPoint(7));

  ptIds->SetId(15,this->PointIds->GetId(4));
  ptIds->SetId(16,this->PointIds->GetId(5));
  ptIds->SetId(17,this->PointIds->GetId(8));
  pts->SetPoint(15,this->Points->GetPoint(4));
  pts->SetPoint(16,this->Points->GetPoint(5));
  pts->SetPoint(17,this->Points->GetPoint(8));

  ptIds->SetId(18,this->PointIds->GetId(5));
  ptIds->SetId(19,this->PointIds->GetId(6));
  ptIds->SetId(20,this->PointIds->GetId(8));
  pts->SetPoint(18,this->Points->GetPoint(5));
  pts->SetPoint(19,this->Points->GetPoint(6));
  pts->SetPoint(20,this->Points->GetPoint(8));

  ptIds->SetId(21,this->PointIds->GetId(6));
  ptIds->SetId(22,this->PointIds->GetId(7));
  ptIds->SetId(23,this->PointIds->GetId(8));
  pts->SetPoint(21,this->Points->GetPoint(6));
  pts->SetPoint(22,this->Points->GetPoint(7));
  pts->SetPoint(23,this->Points->GetPoint(8));

  return 1;
}

//----------------------------------------------------------------------------
void
vtkBiQuadraticQuad::Derivatives (int vtkNotUsed (subId),
                                 double pcoords[3], double *values,
                                 int dim, double *derivs)
{
  double sum[3], weights[9];
  double functionDerivs[18];
  double elemNodes[9][3];
  double *J[3], J0[3], J1[3], J2[3];
  double *JI[3], JI0[3], JI1[3], JI2[3];

  for(int i = 0; i<9; i++)
  {
    this->Points->GetPoint(i, elemNodes[i]);
  }

  this->InterpolationFunctionsPrivate(pcoords,weights);
  this->InterpolationDerivsPrivate(pcoords,functionDerivs);

  // Compute transposed Jacobian and inverse Jacobian
  J[0] = J0; J[1] = J1; J[2] = J2;
  JI[0] = JI0; JI[1] = JI1; JI[2] = JI2;
  for(int k = 0; k<3; k++)
  {
    J0[k] = J1[k] = 0.0;
  }

  for(int i = 0; i<9; i++)
  {
    for(int j = 0; j<2; j++)
    {
      for(int k = 0; k<3; k++)
      {
        J[j][k] += elemNodes[i][k] * functionDerivs[j*9+i];
      }
    }
  }

  // Compute third row vector in transposed Jacobian and normalize it, so that Jacobian determinant stays the same.
  vtkMath::Cross(J0,J1,J2);
  if ( vtkMath::Normalize(J2) == 0.0 || !vtkMath::InvertMatrix(J,JI,3)) //degenerate
  {
    for (int j=0; j < dim; j++ )
    {
      for (int i=0; i < 3; i++ )
      {
        derivs[j*dim + i] = 0.0;
      }
    }
    return;
  }


  // Loop over "dim" derivative values. For each set of values,
  // compute derivatives
  // in local system and then transform into modelling system.
  // First compute derivatives in local x'-y' coordinate system
  for (int j=0; j < dim; j++ )
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (int i=0; i < 9; i++) //loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim*i + j];
      sum[1] += functionDerivs[9 + i] * values[dim*i + j];
    }
//    dBydx = sum[0]*JI[0][0] + sum[1]*JI[0][1];
//    dBydy = sum[0]*JI[1][0] + sum[1]*JI[1][1];

    // Transform into global system (dot product with global axes)
    derivs[3*j] = sum[0]*JI[0][0] + sum[1]*JI[0][1];
    derivs[3*j + 1] = sum[0]*JI[1][0] + sum[1]*JI[1][1];
    derivs[3*j + 2] = sum[0]*JI[2][0] + sum[1]*JI[2][1];
  }
}



//----------------------------------------------------------------------------
// Compute interpolation functions. The first four nodes are the corner
// vertices; the others are mid-edge nodes, the last one is the mid-center
// node.
void vtkBiQuadraticQuad::InterpolationFunctionsPrivate (double pcoords[3],
                                                        double weights[9])
{
  //Normally these coordinates are named r and s, but I chose x and y,
  //because you can easily mark and paste these functions to the
  //gnuplot splot function. :D
  double x = pcoords[0];
  double y = pcoords[1];

  //midedge weights
  weights[0] =  4.0 * (1.0 - x) * (x - 0.5) * (1.0 - y) * (y - 0.5);
  weights[1] = -4.0 *       (x) * (x - 0.5) * (1.0 - y) * (y - 0.5);
  weights[2] =  4.0 *       (x) * (x - 0.5) *       (y) * (y - 0.5);
  weights[3] = -4.0 * (1.0 - x) * (x - 0.5) *       (y) * (y - 0.5);
  //corner weights
  weights[4] =  8.0 *       (x) * (1.0 - x) * (1.0 - y) * (0.5 - y);
  weights[5] = -8.0 *       (x) * (0.5 - x) * (1.0 - y) * (y);
  weights[6] = -8.0 *       (x) * (1.0 - x) *       (y) * (0.5 - y);
  weights[7] =  8.0 * (1.0 - x) * (0.5 - x) * (1.0 - y) * (y);
  //surface center weight
  weights[8] = 16.0 *       (x) * (1.0 - x) * (1.0 - y) * (y);
}

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void vtkBiQuadraticQuad::InterpolationFunctions (double pcoords[3],
                                                 double weights[9])
{
  VTK_LEGACY_REPLACED_BODY(InterpolationFunctions, "VTK 5.2",
                           InterpolateFunctions);

  vtkBiQuadraticQuad::InterpolationFunctionsPrivate(pcoords, weights);
}
#endif

//----------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkBiQuadraticQuad::InterpolationDerivsPrivate (double pcoords[3],
                                                     double derivs[18])
{
  // Coordinate conversion
  double x = pcoords[0];
  double y = pcoords[1];

  // Derivatives in the x-direction
  // edge
  derivs[0] = 4.0 * (1.5 - 2.0 * x) * (1.0 - y) * (y - 0.5);
  derivs[1] =-4.0 * (2.0 * x - 0.5) * (1.0 - y) * (y - 0.5);
  derivs[2] = 4.0 * (2.0 * x - 0.5) *       (y) * (y - 0.5);
  derivs[3] =-4.0 * (1.5 - 2.0 * x) *       (y) * (y - 0.5);
  // midedge
  derivs[4] = 8.0 * (1.0 - 2.0 * x) * (1.0 - y) * (0.5 - y);
  derivs[5] =-8.0 * (0.5 - 2.0 * x) * (1.0 - y) * (y);
  derivs[6] =-8.0 * (1.0 - 2.0 * x) *       (y) * (0.5 - y);
  derivs[7] = 8.0 * (2.0 * x - 1.5) * (1.0 - y) * (y);
  // center
  derivs[8] =16.0 * (1.0 - 2.0 * x) * (1.0 - y) * (y);

  // Derivatives in the y-direction
  // edge
  derivs[9] = 4.0 * (1.0 - x) * (x - 0.5) * (1.5 - 2.0 * y);
  derivs[10]=-4.0 *       (x) * (x - 0.5) * (1.5 - 2.0 * y);
  derivs[11]= 4.0 *       (x) * (x - 0.5) * (2.0 * y - 0.5);
  derivs[12]=-4.0 * (1.0 - x) * (x - 0.5) * (2.0 * y - 0.5);
  // midedge
  derivs[13]= 8.0 *       (x) * (1.0 - x) * (2.0 * y - 1.5);
  derivs[14]=-8.0 *       (x) * (0.5 - x) * (1.0 - 2.0 * y);
  derivs[15]=-8.0 *       (x) * (1.0 - x) * (0.5 - 2.0 * y);
  derivs[16]= 8.0 * (1.0 - x) * (0.5 - x) * (1.0 - 2.0 * y);
  // center
  derivs[17]=16.0 *       (x) * (1.0 - x) * (1.0 - 2.0 * y);

}

//----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void vtkBiQuadraticQuad::InterpolationDerivs (double pcoords[3],
                                              double derivs[18])
{
  VTK_LEGACY_REPLACED_BODY(InterpolationDerivs, "VTK 5.2",
                           InterpolateDerivs);

  vtkBiQuadraticQuad::InterpolationDerivsPrivate(pcoords, derivs);
}
#endif

//----------------------------------------------------------------------------
static double vtkQQuadCellPCoords[27] = { 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                                          1.0, 1.0, 0.0, 0.0, 1.0, 0.0,
                                          0.5, 0.0, 0.0, 1.0, 0.5, 0.0,
                                          0.5, 1.0, 0.0, 0.0, 0.5, 0.0,
                                          0.5, 0.5, 0.0 };

double * vtkBiQuadraticQuad::GetParametricCoords ()
{
  return vtkQQuadCellPCoords;
}

//----------------------------------------------------------------------------
void vtkBiQuadraticQuad::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf (os, indent.GetNextIndent ());
  os << indent << "Quad:\n";
  this->Quad->PrintSelf (os, indent.GetNextIndent ());
  os << indent << "Scalars:\n";
  this->Scalars->PrintSelf (os, indent.GetNextIndent ());
}
