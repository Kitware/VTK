/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticEdge.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadraticEdge.h"

#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkDoubleArray.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkQuadraticEdge);

//----------------------------------------------------------------------------
// Construct the line with two points.
vtkQuadraticEdge::vtkQuadraticEdge()
{
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(2);
  this->Points->SetNumberOfPoints(3);
  this->PointIds->SetNumberOfIds(3);
  for (int i = 0; i < 3; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
  }
  this->Line = vtkLine::New();
}

//----------------------------------------------------------------------------
vtkQuadraticEdge::~vtkQuadraticEdge()
{
  this->Line->Delete();
  this->Scalars->Delete();
}


//----------------------------------------------------------------------------
int vtkQuadraticEdge::EvaluatePosition(double* x, double* closestPoint,
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
  for (minDist2=VTK_DOUBLE_MAX,i=0; i < 2; i++)
  {
    if ( i == 0)
    {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(0));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(2));
    }
    else
    {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(2));
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
    if ( subId == 0 ) //first part
    {
      pcoords[0] = pcoords[0]/2.0;
    }
    else
    {
      pcoords[0] = 0.5 + pcoords[0]/2.0;
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
void vtkQuadraticEdge::EvaluateLocation(int& vtkNotUsed(subId),
                                        double pcoords[3],
                                        double x[3], double *weights)
{
  int i;
  double a0[3], a1[3], a2[3];
  this->Points->GetPoint(0, a0);
  this->Points->GetPoint(1, a1);
  this->Points->GetPoint(2, a2); //midside node

  this->InterpolationFunctions(pcoords,weights);

  for (i=0; i<3; i++)
  {
    x[i] = a0[i]*weights[0] + a1[i]*weights[1] + a2[i]*weights[2];
  }
}

//----------------------------------------------------------------------------
int vtkQuadraticEdge::CellBoundary(int subId, double pcoords[3],
                                   vtkIdList *pts)
{
  return this->Line->CellBoundary(subId, pcoords, pts);
}

//----------------------------------------------------------------------------
static int LinearLines[2][2] = { {0,2}, {2,1} };


void vtkQuadraticEdge::Contour(double value, vtkDataArray *cellScalars,
                               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
                               vtkCellArray *lines, vtkCellArray *polys,
                               vtkPointData *inPd, vtkPointData *outPd,
                               vtkCellData *inCd, vtkIdType cellId,
                               vtkCellData *outCd)
{
  for (int i=0; i < 2; i++) //for each subdivided line
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

// The following arguments were modified to avoid warnings:
// double p1[3], double p2[3], double x[3], double pcoords[3],

int vtkQuadraticEdge::IntersectWithLine(double p1[3], double p2[3],
                                        double tol, double& t,
                                        double x[3], double pcoords[3],
                                        int& subId)
{
  int subTest, numLines=2;

  for (subId=0; subId < numLines; subId++)
  {
    if ( subId == 0)
    {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(0));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(2));
    }
    else
    {
      this->Line->Points->SetPoint(0,this->Points->GetPoint(2));
      this->Line->Points->SetPoint(1,this->Points->GetPoint(1));
    }

    if ( this->Line->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
    {
      return 1;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkQuadraticEdge::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
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

  ptIds->InsertId(3,this->PointIds->GetId(1));
  pts->InsertPoint(3,this->Points->GetPoint(1));

  return 1;
}

//----------------------------------------------------------------------------
void vtkQuadraticEdge::Derivatives(int vtkNotUsed(subId),
                                       double vtkNotUsed(pcoords)[3],
                                       double *vtkNotUsed(values),
                                       int vtkNotUsed(dim),
                                       double *vtkNotUsed(derivs))
{
  // TODO - if the effort is justified, someone should implement a correct
  // version of this method
  vtkErrorMacro( "Derivatives() is not implemented for this cell.");
}

//----------------------------------------------------------------------------
// Clip this quadratic edge using scalar value provided. Like contouring,
// except that it cuts the edge to produce linear line segments.
void vtkQuadraticEdge::Clip(double value, vtkDataArray *cellScalars,
                            vtkIncrementalPointLocator *locator, vtkCellArray *lines,
                            vtkPointData *inPd, vtkPointData *outPd,
                            vtkCellData *inCd, vtkIdType cellId,
                            vtkCellData *outCd, int insideOut)
{
  for (int i=0; i < 2; i++) //for each subdivided line
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
// Compute interpolation functions. Node [2] is the mid-edge node.
void vtkQuadraticEdge::InterpolationFunctions(double pcoords[3],
                                              double weights[3])
{
  double r = pcoords[0];

  weights[0] = 2.0 * (r - 0.5) * (r - 1.0);
  weights[1] = 2.0 * r * (r - 0.5);
  weights[2] = 4.0 * r * (1.0 - r);
}

//----------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkQuadraticEdge::InterpolationDerivs(double pcoords[3], double derivs[3])
{
  double r = pcoords[0];

  derivs[0] = 4.0 * r - 3.0;
  derivs[1] = 4.0 * r - 1.0;
  derivs[2] = 4.0 - r * 8.0;
}

//----------------------------------------------------------------------------
static double vtkQEdgeCellPCoords[9] = {0.0,0.0,0.0, 1.0,0.0,0.0, 0.5,0.0,0.0};
double *vtkQuadraticEdge::GetParametricCoords()
{
  return vtkQEdgeCellPCoords;
}

//----------------------------------------------------------------------------
void vtkQuadraticEdge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os,indent.GetNextIndent());
}
