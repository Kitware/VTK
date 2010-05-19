/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiQuadraticTriangle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBiQuadraticTriangle.h"

#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkQuadraticEdge.h"
#include "vtkTriangle.h"
#include "vtkDoubleArray.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkBiQuadraticTriangle);

//----------------------------------------------------------------------------
// Construct the line with two points.
vtkBiQuadraticTriangle::vtkBiQuadraticTriangle()
{
  this->Edge = vtkQuadraticEdge::New();
  this->Face = vtkTriangle::New();
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(3);

  this->Points->SetNumberOfPoints(7);
  this->PointIds->SetNumberOfIds(7);
  for (int i = 0; i < 7; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }
}


//----------------------------------------------------------------------------
vtkBiQuadraticTriangle::~vtkBiQuadraticTriangle()
{
  this->Edge->Delete();
  this->Face->Delete();
  this->Scalars->Delete();
}



//----------------------------------------------------------------------------
vtkCell *vtkBiQuadraticTriangle::GetEdge(int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 2 ? 2 : edgeId ));
  int p = (edgeId+1) % 3;

  // load point id's
  this->Edge->PointIds->SetId(0,this->PointIds->GetId(edgeId));
  this->Edge->PointIds->SetId(1,this->PointIds->GetId(p));
  this->Edge->PointIds->SetId(2,this->PointIds->GetId(edgeId+3));

  // load coordinates
  this->Edge->Points->SetPoint(0,this->Points->GetPoint(edgeId));
  this->Edge->Points->SetPoint(1,this->Points->GetPoint(p));
  this->Edge->Points->SetPoint(2,this->Points->GetPoint(edgeId+3));

  return this->Edge;
}


//----------------------------------------------------------------------------
// order picked carefully for parametric coordinate conversion
static int LinearTris[6][3] = { {0,3,6}, {6,3,4}, {6,4,5}, {0,6,5}, {3,1,4}, {5,4,2} };

int vtkBiQuadraticTriangle::EvaluatePosition(double* x, double* closestPoint,
                                           int& subId, double pcoords[3],
                                           double& minDist2, double *weights)
{
  double pc[3], dist2, pc0, pc1;
  int ignoreId, i, returnStatus=0, status;
  double tempWeights[3];
  double closest[3];

  pc0 = pc1 = 0;

  //six linear triangles are used
  for (minDist2=VTK_DOUBLE_MAX, i=0; i < 6; i++)
    {
    this->Face->Points->SetPoint(
      0,this->Points->GetPoint(LinearTris[i][0]));
    this->Face->Points->SetPoint(
      1,this->Points->GetPoint(LinearTris[i][1]));
    this->Face->Points->SetPoint(
      2,this->Points->GetPoint(LinearTris[i][2]));

    status = this->Face->EvaluatePosition(x,closest,ignoreId,pc,dist2,
                                          tempWeights);
    if ( status != -1 && dist2 < minDist2 )
      {
      returnStatus = status;
      minDist2 = dist2;
      subId = i;
      pc0 = pc[0];
      pc1 = pc[1];
      if (closestPoint)
        {
        for (int j = 0; j <= 3; j++ )
          {
          closestPoint[j] = closest[j];
          }
        }
      }
    }


  // adjust parametric coordinates
  if ( returnStatus != -1 )
    {
    if ( subId == 0 )
      {
      pcoords[0] = pc0/2.0 + pc1/3.0;
      pcoords[1] = pc1/3.0;
      }
    else if ( subId == 1 )
      {
      pcoords[0] = (1.0/3.0) + pc0/6.0 + pc1/6.0;
      pcoords[1] = (1.0/3.0) + pc0/(-3.0) + pc1/6.0;
      }
    else if ( subId == 2 )
      {
      pcoords[0] = (1.0/3.0) + pc0/6.0 + pc1/(-3.0);
      pcoords[1] = (1.0/3.0) + pc0/6.0 + pc1/6.0;
      }
    else if ( subId == 3 )
      {
      pcoords[0] = pc0/3.0;
      pcoords[1] = pc0/3.0 + pc1*0.5;
      }
    else if ( subId == 4 )
      {
      pcoords[0] = pc0*0.5 + 0.5;
      pcoords[1] = 0.5*pc1;
      }
    else if ( subId == 5 )
      {
      pcoords[0] = 0.5*pc0;
      pcoords[1] = 0.5 + 0.5*pc1;
      }
    pcoords[2] = 1.0 - pcoords[0] - pcoords[1];
    this->InterpolationFunctions(pcoords,weights);
    }

  return returnStatus;
}



//----------------------------------------------------------------------------
void vtkBiQuadraticTriangle::EvaluateLocation(int& vtkNotUsed(subId), 
                                        double pcoords[3], 
                                        double x[3], double *weights)
{
  int i;
  double a0[3], a1[3], a2[3], a3[3], a4[3], a5[3], a6[3];
  this->Points->GetPoint(0, a0);
  this->Points->GetPoint(1, a1);
  this->Points->GetPoint(2, a2);
  this->Points->GetPoint(3, a3);
  this->Points->GetPoint(4, a4);
  this->Points->GetPoint(5, a5);
  this->Points->GetPoint(6, a6);


  this->InterpolationFunctions(pcoords,weights);
  
  for (i=0; i<3; i++) 
    {
    x[i] = a0[i]*weights[0] + a1[i]*weights[1] + a2[i]*weights[2] +
      a3[i]*weights[3] + a4[i]*weights[4] + a5[i]*weights[5] + a6[i]*weights[6];
    }
}



//----------------------------------------------------------------------------
int vtkBiQuadraticTriangle::CellBoundary(int subId, double pcoords[3], 
                                       vtkIdList *pts)
{
  return this->Face->CellBoundary(subId, pcoords, pts);
}



//----------------------------------------------------------------------------
void vtkBiQuadraticTriangle::Contour(double value, 
                                   vtkDataArray* cellScalars, 
                                   vtkIncrementalPointLocator* locator,
                                   vtkCellArray *verts, 
                                   vtkCellArray* lines, 
                                   vtkCellArray* polys, 
                                   vtkPointData* inPd, 
                                   vtkPointData* outPd,
                                   vtkCellData* inCd, 
                                   vtkIdType cellId, 
                                   vtkCellData* outCd)
{
  for ( int i=0; i < 6; i++)
    {
    this->Face->Points->SetPoint(0,this->Points->GetPoint(LinearTris[i][0]));
    this->Face->Points->SetPoint(1,this->Points->GetPoint(LinearTris[i][1]));
    this->Face->Points->SetPoint(2,this->Points->GetPoint(LinearTris[i][2]));

    if ( outPd )
      {
      this->Face->PointIds->SetId(0,this->PointIds->GetId(LinearTris[i][0]));
      this->Face->PointIds->SetId(1,this->PointIds->GetId(LinearTris[i][1]));
      this->Face->PointIds->SetId(2,this->PointIds->GetId(LinearTris[i][2]));
      }

    this->Scalars->SetTuple(0,cellScalars->GetTuple(LinearTris[i][0]));
    this->Scalars->SetTuple(1,cellScalars->GetTuple(LinearTris[i][1]));
    this->Scalars->SetTuple(2,cellScalars->GetTuple(LinearTris[i][2]));

    this->Face->Contour(value, this->Scalars, locator, verts,
                        lines, polys, inPd, outPd, inCd, cellId, outCd);
    }
}



//----------------------------------------------------------------------------
// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkBiQuadraticTriangle::IntersectWithLine(double* p1, 
                                            double* p2, 
                                            double tol, 
                                            double& t,
                                            double* x, 
                                            double* pcoords, 
                                            int& subId)
{
  int subTest, i;
  subId = 0;

  for (i=0; i < 6; i++)
    {
    this->Face->Points->SetPoint(0,this->Points->GetPoint(LinearTris[i][0]));
    this->Face->Points->SetPoint(1,this->Points->GetPoint(LinearTris[i][1]));
    this->Face->Points->SetPoint(2,this->Points->GetPoint(LinearTris[i][2]));

    if (this->Face->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      {
      return 1;
      }
    }

  return 0;
}



//----------------------------------------------------------------------------
int vtkBiQuadraticTriangle::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, 
                                      vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  // Create six linear triangles
  for ( int i=0; i < 6; i++)
    {
    ptIds->InsertId(3*i,this->PointIds->GetId(LinearTris[i][0]));
    pts->InsertPoint(3*i,this->Points->GetPoint(LinearTris[i][0]));
    ptIds->InsertId(3*i+1,this->PointIds->GetId(LinearTris[i][1]));
    pts->InsertPoint(3*i+1,this->Points->GetPoint(LinearTris[i][1]));
    ptIds->InsertId(3*i+2,this->PointIds->GetId(LinearTris[i][2]));
    pts->InsertPoint(3*i+2,this->Points->GetPoint(LinearTris[i][2]));
    }

  return 1;
}



//----------------------------------------------------------------------------
void vtkBiQuadraticTriangle::Derivatives(int vtkNotUsed(subId), 
                                       double pcoords[3], 
                                       double *values, 
                                       int dim, 
                                       double *derivs)
{
  double v0[2], v1[2], v2[2], v3[2], v4[2], v5[2], v6[2];           // Local coordinates of Each point.
  double v10[3], v20[3], lenX;                                      // Reesentation of local Axis
  double x0[3], x1[3], x2[3], x3[3], x4[3], x5[3], x6[3];           // Points of the model
  double n[3], vec20[3], vec30[3], vec40[3], vec50[3], vec60[3];    // Normal and vector of each point. 
  double *J[2], J0[2], J1[2];                                       // Jacobian Matrix
  double *JI[2], JI0[2], JI1[2];                                    // Inverse of the Jacobian Matrix
  double funcDerivs[14], sum[2], dBydx, dBydy;                      // Derivated values
  

  // Project points of BiQuadTriangle into a 2D system
  this->Points->GetPoint(0, x0);
  this->Points->GetPoint(1, x1);
  this->Points->GetPoint(2, x2);
  this->Points->GetPoint(3, x3);
  this->Points->GetPoint(4, x4);
  this->Points->GetPoint(5, x5);
  this->Points->GetPoint(6, x6);
  vtkTriangle::ComputeNormal (x0, x1, x2, n);

  for (int i=0; i < 3; i++)     // Compute the vector for each point
    {
    v10[i] = x1[i] - x0[i];
    vec20[i] = x2[i] - x0[i];
    vec30[i] = x3[i] - x0[i];
    vec40[i] = x4[i] - x0[i];
    vec50[i] = x5[i] - x0[i];
    vec60[i] = x6[i] - x0[i];
    }

  vtkMath::Cross(n,v10,v20); //creates local y' axis

  if ( (lenX=vtkMath::Normalize(v10)) <= 0.0
       || vtkMath::Normalize(v20) <= 0.0 ) //degenerate 
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

  v0[0] = v0[1] = 0.0; //convert points to 2D (i.e., local system)
  v1[0] = lenX; v1[1] = 0.0;
  
  v2[0] = vtkMath::Dot(vec20,v10);
  v2[1] = vtkMath::Dot(vec20,v20);
  
  v3[0] = vtkMath::Dot(vec30,v10);
  v3[1] = vtkMath::Dot(vec30,v20);
 
  v4[0] = vtkMath::Dot(vec40,v10);
  v4[1] = vtkMath::Dot(vec40,v20);

  v5[0] = vtkMath::Dot(vec50,v10);
  v5[1] = vtkMath::Dot(vec50,v20);
  
  v6[0] = vtkMath::Dot(vec60,v10);
  v6[1] = vtkMath::Dot(vec60,v20);

  this->InterpolationDerivs(pcoords, funcDerivs);

  // Compute Jacobian and inverse Jacobian
  J[0] = J0; J[1] = J1;
  JI[0] = JI0; JI[1] = JI1;

  J[0][0] = v0[0]*funcDerivs[0] + v1[0]*funcDerivs[1] +
            v2[0]*funcDerivs[2] + v3[0]*funcDerivs[3] +
            v4[0]*funcDerivs[4] + v5[0]*funcDerivs[5] +
            v6[0]*funcDerivs[6];
  J[0][1] = v0[1]*funcDerivs[0] + v1[1]*funcDerivs[1] +
            v2[1]*funcDerivs[2] + v3[1]*funcDerivs[3] +
            v4[1]*funcDerivs[4] + v5[1]*funcDerivs[5] +
            v6[1]*funcDerivs[6];
  J[1][0] = v0[0]*funcDerivs[7] + v1[0]*funcDerivs[8] +
            v2[0]*funcDerivs[9] + v3[0]*funcDerivs[10] +
            v4[0]*funcDerivs[11] + v5[0]*funcDerivs[12] +
            v6[0]*funcDerivs[13];
  J[1][1] = v0[1]*funcDerivs[7] + v1[1]*funcDerivs[8] +
            v2[1]*funcDerivs[9] + v3[1]*funcDerivs[10] +
            v4[1]*funcDerivs[11] + v5[1]*funcDerivs[12] +
            v6[1]*funcDerivs[13];

  // Compute inverse Jacobian, return if Jacobian is singular
  if (!vtkMath::InvertMatrix(J,JI,2))
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
    sum[0] = sum[1] = 0.0;
    for (int i=0; i < 7; i++) //loop over interp. function derivatives
      {
      sum[0] += funcDerivs[i] * values[dim*i + j]; 
      sum[1] += funcDerivs[7 + i] * values[dim*i + j];
      }
    dBydx = sum[0]*JI[0][0] + sum[1]*JI[0][1];
    dBydy = sum[0]*JI[1][0] + sum[1]*JI[1][1];

    // Transform into global system (dot product with global axes)
    derivs[3*j] = dBydx * v10[0] + dBydy * v20[0];
    derivs[3*j + 1] = dBydx * v10[1] + dBydy * v20[1];
    derivs[3*j + 2] = dBydx * v10[2] + dBydy * v20[2];
    }
}



//----------------------------------------------------------------------------
// Clip this quadratic triangle using the scalar value provided. Like 
// contouring, except that it cuts the triangle to produce other quads
// and triangles.
void vtkBiQuadraticTriangle::Clip(double value, 
                                vtkDataArray* cellScalars, 
                                vtkIncrementalPointLocator* locator,
                                vtkCellArray* polys,
                                vtkPointData* inPd, 
                                vtkPointData* outPd,
                                vtkCellData* inCd, 
                                vtkIdType cellId, 
                                vtkCellData* outCd,
                                int insideOut)
{
  for ( int i=0; i < 6; i++)
    {
    this->Face->Points->SetPoint(0,this->Points->GetPoint(LinearTris[i][0]));
    this->Face->Points->SetPoint(1,this->Points->GetPoint(LinearTris[i][1]));
    this->Face->Points->SetPoint(2,this->Points->GetPoint(LinearTris[i][2]));

    this->Face->PointIds->SetId(0,this->PointIds->GetId(LinearTris[i][0]));
    this->Face->PointIds->SetId(1,this->PointIds->GetId(LinearTris[i][1]));
    this->Face->PointIds->SetId(2,this->PointIds->GetId(LinearTris[i][2]));

    this->Scalars->SetTuple(0,cellScalars->GetTuple(LinearTris[i][0]));
    this->Scalars->SetTuple(1,cellScalars->GetTuple(LinearTris[i][1]));
    this->Scalars->SetTuple(2,cellScalars->GetTuple(LinearTris[i][2]));

    this->Face->Clip(value, this->Scalars, locator, polys, inPd, outPd, 
                     inCd, cellId, outCd, insideOut);
    }
}



//----------------------------------------------------------------------------
// Compute maximum parametric distance to cell
double vtkBiQuadraticTriangle::GetParametricDistance(double pcoords[3])
{
  int i;
  double pDist, pDistMax=0.0;
  double pc[3];

  pc[0] = pcoords[0];
  pc[1] = pcoords[1];
  pc[2] = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
    {
    if ( pc[i] < 0.0 ) 
      {
      pDist = -pc[i];
      }
    else if ( pc[i] > 1.0 ) 
      {
      pDist = pc[i] - 1.0;
      }
    else //inside the cell in the parametric direction
      {
      pDist = 0.0;
      }
    if ( pDist > pDistMax )
      {
      pDistMax = pDist;
      }
    }
  
  return pDistMax;
}



//----------------------------------------------------------------------------
// Compute interpolation functions. The first three nodes are the triangle
// vertices; the next three nodes are mid-edge nodes; the last node is the mid-cell node.
void vtkBiQuadraticTriangle::InterpolationFunctions(double pcoords[3], 
                                                  double weights[7])
{
  double r = pcoords[0];
  double s = pcoords[1];

  weights[0] = 1.0 - 3.0*(r + s) + 2.0*(r*r + s*s) + 7.0*r*s - 3.0*r*s*(r + s);
  weights[1] = r*(-1.0 + 2.0*r + 3.0*s - 3.0*s*(r + s));
  weights[2] = s*(-1.0 + 3.0*r + 2.0*s - 3.0*r*(r + s));
  weights[3] = 4.0*r*(1.0 - r - 4.0*s + 3.0*s*(r + s));
  weights[4] = 4.0*r*s*(-2.0 + 3.0*(r + s));
  weights[5] = 4.0*s*(1.0 - 4.0*r - s + 3.0*r*(r + s));
  weights[6] = 27.0*r*s*(1.0 - r - s);
}



//----------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkBiQuadraticTriangle::InterpolationDerivs(double pcoords[3], 
                                               double derivs[14])
{
  double r = pcoords[0];
  double s = pcoords[1];

  // r-derivatives
  derivs[0] = -3.0 + 4.0*r + 7.0*s - 6.0*r*s - 3.0*s*s;
  derivs[1] = -1.0 + 4.0*r + 3.0*s - 6.0*r*s - 3.0*s*s;
  derivs[2] = 3.0*s*(1.0 - s - 2.0*r); 
  derivs[3] = 4.0*(1.0 - 2.0*r - 4.0*s + 6.0*r*s + 3.0*s*s);
  derivs[4] = 4.0*s*(-2.0 + 6.0*r + 3.0*s);
  derivs[5] = 4.0*s*(-4.0 + 6.0*r + 3.0*s);
  derivs[6] = 27.0*s*(1.0 - 2.0*r - s);

  // s-derivatives
  derivs[7] = -3.0 + 7.0*r + 4.0*s - 6.0*r*s - 3.0*r*r;
  derivs[8] = 3.0*r*(1.0 - r - 2.0*s);
  derivs[9] = -1.0 + 3.0*r + 4.0*s - 6.0*r*s - 3.0*r*r;
  derivs[10] = 4.0*r*(-4.0 + 3.0*r + 6.0*s);
  derivs[11] = 4.0*r*(-2.0 + 3.0*r + 6.0*s);
  derivs[12] = 4.0*(1.0 - 4.0*r -2.0*s + 6.0*r*s + 3.0*r*r); 
  derivs[13] = 27.0*r*(1.0 - r - 2.0*s);
}



//----------------------------------------------------------------------------
static double vtkBiQTriangleCellPCoords[21] = {
  0.0,0.0,0.0, 1.0,0.0,0.0, 0.0,1.0,0.0,
  0.5,0.0,0.0, 0.5,0.5,0.0, 0.0,0.5,0.0, (1.0/3.0),(1.0/3.0),0.0};
double *vtkBiQuadraticTriangle::GetParametricCoords()
{
  return vtkBiQTriangleCellPCoords;
}



//----------------------------------------------------------------------------
void vtkBiQuadraticTriangle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Edge: "    << this->Edge    << endl;
  os << indent << "Face: "    << this->Face    << endl;
  os << indent << "Scalars: " << this->Scalars << endl;
}
