/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubicLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCubicLine.h"

#include "vtkNonLinearCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLine.h"
#include "vtkDoubleArray.h"
#include "vtkPoints.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkCubicLine);

//----------------------------------------------------------------------------
// Construct the line with four points.
vtkCubicLine::vtkCubicLine()
{
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(4);
  this->Points->SetNumberOfPoints(4);
  this->PointIds->SetNumberOfIds(4);
  for (int i = 0; i < 4; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }
  this->Line = vtkLine::New();
}
//----------------------------------------------------------------------------
// Delete the Line
vtkCubicLine::~vtkCubicLine()
{
  this->Line->Delete();
  this->Scalars->Delete();
}


//----------------------------------------------------------------------------
static const int VTK_NO_INTERSECTION=0;
static const int VTK_YES_INTERSECTION=2;
static const int VTK_ON_LINE=3;

int vtkCubicLine::EvaluatePosition(double x[3], double* closestPoint,
                             int& subId, double pcoords[3],
                             double& minDist2, double *weights)
{
  double closest[3];
  double pc[3], dist2;
  int ignoreId, i, returnStatus, status;
  double lineWeights[2];

  pcoords[1] = pcoords[2] = 0.0;

  returnStatus = -1;
  weights[0] = 0.0;
  for (minDist2=VTK_DOUBLE_MAX,i=0; i < 3; i++)
    {
    if ( i == 0)
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(0));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(2));
      }
    else if (i == 1)
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(2));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(3));
      }
    else
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(3));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(1));
      }


    status = this->Line->EvaluatePosition(x,closest,ignoreId,pc,
                                          dist2,lineWeights);
    if ( status != -1 && dist2 < minDist2 )
      {
      returnStatus = status;
      minDist2 = dist2;
      subId = i;
      pcoords[0] = pc[0];
      }
    }

  // adjust parametric coordinate
  if ( returnStatus != -1 )
    {
    if ( subId == 0 ) //first part  : -1 <= pcoords <= -1/3
      {
      pcoords[0] = pcoords[0]*(2.0/3.0) - 1;
      }
    else if ( subId == 1 ) //second part  : -1/3 <= pcoords <= 1/3
      {
      pcoords[0] = pcoords[0]*(2.0/3.0) -(1.0/3.0) ;
      }
    else              // third part : 1/3 <= pcoords <= 1
      {
      pcoords[0] = pcoords[0]*(2.0/3.0) + (1.0/3.0);
      }
    if(closestPoint!=0)
      {
      // Compute both closestPoint and weights
      this->EvaluateLocation(subId,pcoords,closestPoint,weights);
      }
    else
      {
      // Compute weights only
      this->InterpolationFunctions(pcoords,weights);
      }
    }

  return returnStatus;
}



//----------------------------------------------------------------------------
void vtkCubicLine::EvaluateLocation(int& vtkNotUsed(subId), double pcoords[3],
                               double x[3], double *weights)
{
  int i;
  double a0[3], a1[3], a2[3], a3[3];
  this->Points->GetPoint(0, a0);
  this->Points->GetPoint(1, a1);
  this->Points->GetPoint(2, a2); //first midside node
  this->Points->GetPoint(3, a3); //second midside node

  this->InterpolationFunctions(pcoords,weights);


  for (i=0; i<3; i++)
    {
    x[i] = a0[i]*weights[0] + a1[i]*weights[1] + a2[i]*weights[2] + a3[i]*weights[3];
    }

}





//----------------------------------------------------------------------------
int vtkCubicLine::CellBoundary(int vtkNotUsed(subId), double pcoords[3],
                          vtkIdList *pts)
{
  pts->SetNumberOfIds(1);

  if ( pcoords[0] >= 0.0 )
    {
    pts->SetId(0,this->PointIds->GetId(1));   // The edge points IDs are 0 and 1.
    if ( pcoords[0] > 1.0 )
      {
      return 0;
      }
    else
      {
      return 1;
      }
    }
  else
    {
    pts->SetId(0,this->PointIds->GetId(0));
    if ( pcoords[0] < -1.0 )
      {
      return 0;
      }
    else
      {
      return 1;
      }
    }
}


//LinearLines for the Contour and the Clip Algorithm
//-----------------------------------------------------------------------------
static int LinearLines[3][2] = { {0,2}, {2,3}, {3,1} };


void vtkCubicLine::Contour(double value, vtkDataArray *cellScalars,
                               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
                               vtkCellArray *lines, vtkCellArray *polys,
                               vtkPointData *inPd, vtkPointData *outPd,
                               vtkCellData *inCd, vtkIdType cellId,
                               vtkCellData *outCd)
{
  for (int i=0; i < 3; i++) //for each subdivided line
    {
    for ( int j=0; j<2; j++) //for each of the four vertices of the line
      {
      this->Line->Points->SetPoint(j,this->Points->GetPoint(LinearLines[i][j]));
      this->Line->PointIds->SetId(j,this->PointIds->GetId(LinearLines[i][j]));
      this->Scalars->SetValue(j,cellScalars->GetTuple1(LinearLines[i][j]));
      }
    this->Line->Contour(value, this->Scalars, locator, verts,
                       lines, polys, inPd, outPd, inCd, cellId, outCd);
    }
}




//----------------------------------------------------------------------------
// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkCubicLine::IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                               double x[3], double pcoords[3], int& subId)
{


  int subTest, numLines=3;

  for (subId=0; subId < numLines; subId++)
    {
    if ( subId == 0)
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(0));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(2));
      }
    else if (subId == 1)
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(2));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(3));
      }
    else
      {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(3));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(1));
      }

    if ( this->Line->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      {
      // adjust parametric coordinate

      if ( subId == 0 )      //first part  : -1 <= pcoords <= -1/3
        {
        pcoords[0] = pcoords[0]*(2.0/3.0) - 1;
        }
      else if ( subId == 1 ) //second part  : -1/3 <= pcoords <= 1/3
        {
        pcoords[0] = pcoords[0]*(2.0/3.0) - (1.0/3.0) ;
        }
      else                   // third part : 1/3 <= pcoords <= 1
        {
        pcoords[0] = pcoords[0]*(2.0/3.0) + (1.0/3.0);
        }

      return 1;
      }
    }

  return 0;
}




//----------------------------------------------------------------------------
int vtkCubicLine::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                         vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  // The first line
  ptIds->InsertId(0,this->PointIds->GetId(0));
  pts->InsertPoint(0,this->Points->GetPoint(0));

  ptIds->InsertId(1,this->PointIds->GetId(2));
  pts->InsertPoint(1,this->Points->GetPoint(2));

  // The second line
  ptIds->InsertId(2,this->PointIds->GetId(2));
  pts->InsertPoint(2,this->Points->GetPoint(2));

  ptIds->InsertId(3,this->PointIds->GetId(3));
  pts->InsertPoint(3,this->Points->GetPoint(3));


  // The third line
  ptIds->InsertId(4,this->PointIds->GetId(3));
  pts->InsertPoint(4,this->Points->GetPoint(3));

  ptIds->InsertId(5,this->PointIds->GetId(1));
  pts->InsertPoint(5,this->Points->GetPoint(1));
  return 1;
}


//----------------------------------------------------------------------------
void vtkCubicLine::Derivatives(int vtkNotUsed(subId),
                                       double pcoords[3],
                                       double *values,
                                       int dim,
                                       double *derivs)
{
  double v0, v1, v2, v3;                // Local coordinates of Each point.
  double v10[3], lenX;                  // Reesentation of local Axis
  double x0[3], x1[3], x2[3], x3[3];    // Points of the model
  double vec20[3], vec30[3];            // Normal and vector of each point.
  double J;                             // Jacobian Matrix
  double JI;                            // Inverse of the Jacobian Matrix
  double funcDerivs[4], sum, dBydx;     // Derivated values


  // Project points of vtkCubicLine into a 1D system
  this->Points->GetPoint(0, x0);
  this->Points->GetPoint(1, x1);
  this->Points->GetPoint(2, x2);
  this->Points->GetPoint(3, x3);

  for (int i=0; i < 3; i++)                    // Compute the vector for each point
    {
    v10[i] = x1[i] - x0[i];
    vec20[i] = x2[i] - x0[i];
    vec30[i] = x3[i] - x0[i];
    }


  if ( (lenX=vtkMath::Normalize(v10)) <= 0.0 ) //degenerate
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

  v0 =  0.0; //convert points to 1D (i.e., local system)

  v1 = lenX;

  v2 = vtkMath::Dot(vec20,v10);

  v3 = vtkMath::Dot(vec30,v10);

  this->InterpolationDerivs(pcoords, funcDerivs);

  J = v0*funcDerivs[0] + v1*funcDerivs[1] +
            v2*funcDerivs[2] + v3*funcDerivs[3];

  // Compute inverse Jacobian, return if Jacobian is singular

  if(J != 0)
    {
    JI = 1.0 / J;
    }
  else
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
  // First compute derivatives in local x' coordinate system
  for (int j=0; j < dim; j++ )
    {
    sum = 0.0;
    for (int i=0; i < 4; i++) //loop over interp. function derivatives
      {
      sum += funcDerivs[i] * values[dim*i + j];
      }

    dBydx = sum*JI;

    // Transform into global system (dot product with global axes)
    derivs[3*j] = dBydx * v10[0] ;
    derivs[3*j + 1] = dBydx * v10[1] ;
    derivs[3*j + 2] = dBydx * v10[2] ;
    }
}




//--------------------------------------------------------------------------
// Clip this line using scalar value provided. Like contouring, except
// that it cuts the line to produce other lines.
void vtkCubicLine::Clip(double value, vtkDataArray *cellScalars,
                   vtkIncrementalPointLocator *locator, vtkCellArray *lines,
                   vtkPointData *inPd, vtkPointData *outPd,
                   vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                   int insideOut)
{
   for (int i=0; i < 3; i++) //for each subdivided line
    {
    for ( int j=0; j<2; j++) //for each of the four vertices of the line
      {
      this->Line->Points->SetPoint(j,this->Points->GetPoint(LinearLines[i][j]));
      this->Line->PointIds->SetId(j,this->PointIds->GetId(LinearLines[i][j]));
      this->Scalars->SetValue(j,cellScalars->GetTuple1(LinearLines[i][j]));
      }
    this->Line->Clip(value, this->Scalars, locator, lines, inPd, outPd,
                     inCd, cellId, outCd, insideOut);
    }
}


//----------------------------------------------------------------------------
//
// Compute interpolation functions
//
void vtkCubicLine::InterpolationFunctions(double pcoords[3], double weights[4]) // N2 and N3 are the middle points
{
  // pcoords[0] = t, weights need to be set in accordance with the definition of the standard cubic line finite element
  double t = pcoords[0];

  weights[0] =  (9.0/16.0)*(1.0 - t)*(t + (1.0/3.0))*(t - (1.0/3.0));
  weights[1] = (-9.0/16.0)*(1.0 + t)*((1.0/3.0) - t)*(t + (1.0/3.0));
  weights[2] =  (27.0/16.0)*(t - 1.0)*(t + 1.0)*(t - (1.0/3.0));
  weights[3] = (-27.0/16.0)*(t - 1.0)*(t + 1.0)*(t + (1.0/3.0));
}




//----------------------------------------------------------------------------
void vtkCubicLine::InterpolationDerivs(double pcoords[3], double derivs[4])  //N2 and N3 are the middle points
{
  double t = pcoords[0];

  derivs[0] = (1.0/16.0)*(1.0 + 18.0*t - 27.0*t*t);
  derivs[1] = (1.0/16.0)*(-1.0 + 18.0*t + 27.0*t*t);
  derivs[2] = (1.0/16.0)*(-27.0 - 18.0*t + 81.0*t*t);
  derivs[3] = (1.0/16.0)*(27.0 - 18.0*t - 81.0*t*t);

}



//----------------------------------------------------------------------------
static double vtkCubicLineCellPCoords[12] = {-1.0,0.0,0.0, 1.0,0.0,0.0, -(1.0/3.0),0.0,0.0, (1.0/3.0),0.0,0.0};
double *vtkCubicLine::GetParametricCoords()
{
  return vtkCubicLineCellPCoords;
}


//----------------------------------------------------------------------------
double vtkCubicLine::GetParametricDistance(double pcoords[3])
{

  double pc;

  pc = pcoords[0];

  if ( pc <= -1.0)
    {
    return  pc * (-1.0) - 1.0;
    }
  else if (pc >= 1.0)
    {
    return pc - 1.0;
    }

  return pc;    // the parametric coordintate lies between -1.0 and 1.0.
}


//----------------------------------------------------------------------------
void vtkCubicLine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Line: " << this->Line << endl;
}
