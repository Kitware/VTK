/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticQuad.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadraticQuad.h"
#include "vtkPolyData.h"
#include "vtkPointLocator.h"
#include "vtkMath.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuad.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkQuadraticQuad, "1.11");
vtkStandardNewMacro(vtkQuadraticQuad);

// Construct the line with two points.
vtkQuadraticQuad::vtkQuadraticQuad()
{
  this->Edge = vtkQuadraticEdge::New();
  this->Quad = vtkQuad::New();
  this->PointData = vtkPointData::New();
  this->CellData = vtkCellData::New();
  this->Scalars = vtkFloatArray::New();
  this->Scalars->SetNumberOfTuples(4);

  // We add a fictitious ninth point in order to process the cell. The ninth
  // point is in the center of the cell.
  int i;
  this->Points->SetNumberOfPoints(9);
  this->PointIds->SetNumberOfIds(9);
  for (i = 0; i < 9; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }
  this->Points->SetNumberOfPoints(8);
  this->PointIds->SetNumberOfIds(8);
}

vtkQuadraticQuad::~vtkQuadraticQuad()
{
  this->Edge->Delete();
  this->Quad->Delete();

  this->PointData->Delete();
  this->CellData->Delete();
  this->Scalars->Delete();
}

vtkCell *vtkQuadraticQuad::MakeObject()
{
  vtkQuadraticQuad *cell = vtkQuadraticQuad::New();
  cell->DeepCopy(this);
  return (vtkCell *)cell;
}

vtkCell *vtkQuadraticQuad::GetEdge(int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 3 ? 3 : edgeId ));
  int p = (edgeId+1) % 3;

  // load point id's
  this->Edge->PointIds->SetId(0,this->PointIds->GetId(edgeId));
  this->Edge->PointIds->SetId(1,this->PointIds->GetId(p));
  this->Edge->PointIds->SetId(2,this->PointIds->GetId(edgeId+4));

  // load coordinates
  this->Edge->Points->SetPoint(0,this->Points->GetPoint(edgeId));
  this->Edge->Points->SetPoint(1,this->Points->GetPoint(p));
  this->Edge->Points->SetPoint(2,this->Points->GetPoint(edgeId+4));

  return this->Edge;
}

static int LinearQuads[4][4] = { {0,4,8,7}, {8,4,1,5}, 
                                 {8,5,2,6}, {7,8,6,3} };

void vtkQuadraticQuad::Subdivide(float *weights)
{
  int i, j;
  float pc[3], x[3];

  pc[0] = pc[1] = 0.5;
  this->InterpolationFunctions(pc, weights);
  
  float *p = ((vtkFloatArray *)this->Points->GetData())->GetPointer(0);

  for (j=0; j<3; j++) 
    {
    x[j] = 0.0;
    for (i=0; i<8; i++) 
      {
      x[j] += p[3*i+j] * weights[i];
      }
    }
  this->Points->SetPoint(8,x);
}

int vtkQuadraticQuad::EvaluatePosition(float* x, 
                                       float* closestPoint, 
                                       int& subId, float pcoords[3],
                                       float& minDist2, 
                                       float *weights)
{
  float pc[3], dist2;
  int ignoreId, i, returnStatus=0, status;
  float tempWeights[4];
  float closest[3];

  // compute the midquad node
  this->Subdivide(weights);

  //four linear quads are used
  for (minDist2=VTK_LARGE_FLOAT, i=0; i < 4; i++)
    {
    this->Quad->Points->SetPoint(
      0,this->Points->GetPoint(LinearQuads[i][0]));
    this->Quad->Points->SetPoint(
      1,this->Points->GetPoint(LinearQuads[i][1]));
    this->Quad->Points->SetPoint(
      2,this->Points->GetPoint(LinearQuads[i][2]));
    this->Quad->Points->SetPoint(
      3,this->Points->GetPoint(LinearQuads[i][3]));

    status = this->Quad->EvaluatePosition(x,closest,ignoreId,pc,dist2,
                                          tempWeights);
    if ( status != -1 && dist2 < minDist2 )
      {
      returnStatus = status;
      minDist2 = dist2;
      subId = i;
      pcoords[0] = pc[0];
      pcoords[1] = pc[1];
      }
    }

  // adjust parametric coordinates
  if ( returnStatus != -1 )
    {
    if ( subId == 0 )
      {
      pcoords[0] /= 2.0;
      pcoords[1] /= 2.0;
      }
    else if ( subId == 1 )
      {
      pcoords[0] = 0.5 + (pcoords[0]/2.0);
      pcoords[1] /= 2.0;
      }
    else if ( subId == 2 )
      {
      pcoords[0] = 0.5 + (pcoords[0]/2.0);
      pcoords[1] = 0.5 + (pcoords[1]/2.0);
      }
    else 
      {
      pcoords[0] /= 2.0;
      pcoords[1] = 0.5 + (pcoords[1]/2.0);
      }
    pcoords[2] = 0.0;
    this->EvaluateLocation(subId,pcoords,closestPoint,weights);
    }

  return returnStatus;
}

void vtkQuadraticQuad::EvaluateLocation(int& vtkNotUsed(subId), 
                                        float pcoords[3], 
                                        float x[3], float *weights)
{
  int i, j;
  float *p = ((vtkFloatArray *)this->Points->GetData())->GetPointer(0);

  this->InterpolationFunctions(pcoords,weights);
  
  for (j=0; j<3; j++) 
    {
    x[j] = 0.0;
    for (i=0; i<8; i++) 
      {
      x[j] += p[3*i+j] * weights[i];
      }
    }
}

int vtkQuadraticQuad::CellBoundary(int subId, float pcoords[3], vtkIdList *pts)
{
  return this->Quad->CellBoundary(subId, pcoords, pts);
}

void vtkQuadraticQuad::InterpolateAttributes(vtkPointData *inPd, 
                                             vtkCellData *inCd, 
                                             vtkIdType cellId,
                                             float *weights)
{
  this->PointData->CopyAllocate(inPd,9);
  this->CellData->CopyAllocate(inCd,4);
  
  // copy the point data over into point ids 0->7
  for (int i=0; i<8; i++)
    {
    this->PointData->CopyData(inPd,this->PointIds->GetId(i),i);
    }

  // now interpolate the center point
  this->PointIds->SetNumberOfIds(8);
  this->PointData->InterpolatePoint(inPd, 8, this->PointIds, weights);
  this->PointIds->SetNumberOfIds(9);
  this->PointIds->SetId(8,8);
  
  // copy the cell data over to the linear cell
  this->CellData->CopyData(inCd,cellId,0);
  
}

void vtkQuadraticQuad::Contour(float value, 
                               vtkDataArray* vtkNotUsed(cellScalars), 
                               vtkPointLocator* locator, 
                               vtkCellArray *verts, 
                               vtkCellArray* lines, 
                               vtkCellArray* polys, 
                               vtkPointData* inPd, 
                               vtkPointData* outPd,
                               vtkCellData* inCd, 
                               vtkIdType cellId, 
                               vtkCellData* outCd)
{
  float weights[8];

  //first define the midquad point
  this->Subdivide(weights);
  
  //interpolate point and cell data
  this->InterpolateAttributes(inPd,inCd,cellId,weights);
  
  //contour each linear quad separately
  vtkDataArray *localScalars = this->PointData->GetScalars();
  for (int i=0; i<4; i++)
    {
    for (int j=0; j<4; j++)
      {
      this->Quad->Points->SetPoint(j,this->Points->GetPoint(LinearQuads[i][j]));
      this->Quad->PointIds->SetId(j,this->PointIds->GetId(LinearQuads[i][j]));
      this->Scalars->SetValue(j,localScalars->GetTuple1(LinearQuads[i][j]));
      }
    
    this->Quad->Contour(value,this->Scalars,locator,verts,lines,polys,
                        this->PointData,outPd,this->CellData,0,outCd);
    }
}

// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkQuadraticQuad::IntersectWithLine(float* p1, 
                                        float* p2, 
                                        float tol, 
                                        float& t,
                                        float* x, 
                                        float* pcoords, 
                                        int& subId)
{
  int subTest, i;
  subId = 0;
  float weights[8];

  //first define the midquad point
  this->Subdivide(weights);
  
  //intersect the four linear quads
  for (i=0; i < 4; i++)
    {
    this->Quad->Points->SetPoint(0,this->Points->GetPoint(LinearQuads[i][0]));
    this->Quad->Points->SetPoint(1,this->Points->GetPoint(LinearQuads[i][1]));
    this->Quad->Points->SetPoint(2,this->Points->GetPoint(LinearQuads[i][2]));
    this->Quad->Points->SetPoint(3,this->Points->GetPoint(LinearQuads[i][3]));

    if (this->Quad->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      {
      return 1;
      }
    }

  return 0;
}

int vtkQuadraticQuad::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds, 
                                  vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  // Create six linear triangles: one at each corner and two
  // to cover the remaining quadrilateral.
  
  // First the corner vertices
  ptIds->InsertId(0,this->PointIds->GetId(0));
  ptIds->InsertId(1,this->PointIds->GetId(4));
  ptIds->InsertId(2,this->PointIds->GetId(7));
  pts->InsertPoint(0,this->Points->GetPoint(0));
  pts->InsertPoint(1,this->Points->GetPoint(4));
  pts->InsertPoint(2,this->Points->GetPoint(7));

  ptIds->InsertId(3,this->PointIds->GetId(4));
  ptIds->InsertId(4,this->PointIds->GetId(1));
  ptIds->InsertId(5,this->PointIds->GetId(5));
  pts->InsertPoint(3,this->Points->GetPoint(4));
  pts->InsertPoint(4,this->Points->GetPoint(1));
  pts->InsertPoint(5,this->Points->GetPoint(5));

  ptIds->InsertId(6,this->PointIds->GetId(5));
  ptIds->InsertId(7,this->PointIds->GetId(2));
  ptIds->InsertId(8,this->PointIds->GetId(6));
  pts->InsertPoint(6,this->Points->GetPoint(5));
  pts->InsertPoint(7,this->Points->GetPoint(2));
  pts->InsertPoint(8,this->Points->GetPoint(6));

  ptIds->InsertId(9,this->PointIds->GetId(6));
  ptIds->InsertId(10,this->PointIds->GetId(3));
  ptIds->InsertId(11,this->PointIds->GetId(7));
  pts->InsertPoint(9,this->Points->GetPoint(6));
  pts->InsertPoint(10,this->Points->GetPoint(3));
  pts->InsertPoint(11,this->Points->GetPoint(7));

  // Now the two remaining triangles
  // Choose the triangulation that minimizes the edge length
  // across the cell.
  float *x4 = this->Points->GetPoint(4);
  float *x5 = this->Points->GetPoint(5);
  float *x6 = this->Points->GetPoint(6);
  float *x7 = this->Points->GetPoint(7);
  
  if ( vtkMath::Distance2BetweenPoints(x4,x6) <=
       vtkMath::Distance2BetweenPoints(x5,x7) )
    {
    ptIds->InsertId(12,this->PointIds->GetId(4));
    ptIds->InsertId(13,this->PointIds->GetId(6));
    ptIds->InsertId(14,this->PointIds->GetId(7));
    pts->InsertPoint(12,this->Points->GetPoint(4));
    pts->InsertPoint(13,this->Points->GetPoint(6));
    pts->InsertPoint(14,this->Points->GetPoint(7));

    ptIds->InsertId(15,this->PointIds->GetId(4));
    ptIds->InsertId(16,this->PointIds->GetId(5));
    ptIds->InsertId(17,this->PointIds->GetId(6));
    pts->InsertPoint(15,this->Points->GetPoint(4));
    pts->InsertPoint(16,this->Points->GetPoint(5));
    pts->InsertPoint(17,this->Points->GetPoint(6));
    }
  else
    {
    ptIds->InsertId(12,this->PointIds->GetId(5));
    ptIds->InsertId(13,this->PointIds->GetId(6));
    ptIds->InsertId(14,this->PointIds->GetId(7));
    pts->InsertPoint(12,this->Points->GetPoint(5));
    pts->InsertPoint(13,this->Points->GetPoint(6));
    pts->InsertPoint(14,this->Points->GetPoint(7));

    ptIds->InsertId(15,this->PointIds->GetId(5));
    ptIds->InsertId(16,this->PointIds->GetId(7));
    ptIds->InsertId(17,this->PointIds->GetId(4));
    pts->InsertPoint(15,this->Points->GetPoint(5));
    pts->InsertPoint(16,this->Points->GetPoint(7));
    pts->InsertPoint(17,this->Points->GetPoint(4));
    }

  return 1;
}

void vtkQuadraticQuad::Derivatives(int vtkNotUsed(subId), 
                                   float pcoords[3], float *values, 
                                   int dim, float *derivs)
{
  float *x0, *x1, *x2, deltaX[3], weights[3];
  int i, j;

  x0 = this->Points->GetPoint(0);
  x1 = this->Points->GetPoint(1);
  x2 = this->Points->GetPoint(2);

  this->InterpolationFunctions(pcoords,weights);
  this->InterpolationDerivs(pcoords,derivs);
  
  for (i=0; i<3; i++)
    {
    deltaX[i] = x1[i] - x0[i]              - x2[i];
    }
  for (i=0; i<dim; i++) 
    {
    for (j=0; j<3; j++)
      {
      if ( deltaX[j] != 0 )
        {
        derivs[3*i+j] = (values[2*i+1] - values[2*i]) / deltaX[j];
        }
      else
        {
        derivs[3*i+j] =0;
        }
      }
    }
}


// Clip this quadratic quad using scalar value provided. Like contouring, 
// except that it cuts the quad to produce other quads and triangles.
void vtkQuadraticQuad::Clip(float value, vtkDataArray* vtkNotUsed(cellScalars), 
                            vtkPointLocator* locator, vtkCellArray* polys,
                            vtkPointData* inPd, vtkPointData* outPd,
                            vtkCellData* inCd, vtkIdType cellId, 
                            vtkCellData* outCd, int insideOut)
{
  float weights[8];

  //first define the midquad point
  this->Subdivide(weights);
  
  //interpolate point and cell data
  this->InterpolateAttributes(inPd,inCd,cellId,weights);
  
  //contour each linear quad separately
  vtkDataArray *localScalars = this->PointData->GetScalars();
  for (int i=0; i<4; i++)
    {
    for ( int j=0; j<4; j++) //for each of the four vertices of the linear quad
      {
      this->Quad->Points->SetPoint(j,this->Points->GetPoint(LinearQuads[i][j]));
      this->Quad->PointIds->SetId(j,this->PointIds->GetId(LinearQuads[i][j]));
      this->Scalars->SetTuple(j,localScalars->GetTuple(LinearQuads[i][j]));
      }

    this->Quad->Clip(value,this->Scalars,locator,polys,this->PointData,
                     outPd,this->CellData,0,outCd,insideOut);
    }
}

// Compute interpolation functions. The first four nodes are the corner
// vertices; the others are mid-edge nodes.
void vtkQuadraticQuad::InterpolationFunctions(float pcoords[3], 
                                              float weights[8])
{
  //VTK needs parametric coordinates to be between (0,1). Isoparametric
  //shape functions are formulated between (-1,1). Here we do a 
  //coordinate system conversion from (0,1) to (-1,1).
  float r = 2.0*(pcoords[0]-0.5);
  float s = 2.0*(pcoords[1]-0.5);

  //midedge weights
  weights[4] = 0.5 * (1.0 - r*r) * (1.0 - s);
  weights[5] = 0.5 * (1.0 + r) * (1.0 - s*s);
  weights[6] = 0.5 * (1.0 + r*r) * (1.0 + s);
  weights[7] = 0.5 * (1.0 - r) * (1.0 - s*s);

  //corner
  weights[0] = 0.25 * (1.0 - r) * (1.0 - s) - 0.5*(weights[7]+weights[4]);
  weights[1] = 0.25 * (1.0 + r) * (1.0 - s) - 0.5*(weights[4]+weights[5]);
  weights[2] = 0.25 * (1.0 + r) * (1.0 + s) - 0.5*(weights[5]+weights[6]);
  weights[3] = 0.25 * (1.0 - r) * (1.0 + s) - 0.5*(weights[6]+weights[7]);;
}

// Derivatives in parametric space.
void vtkQuadraticQuad::InterpolationDerivs(float pcoords[3], float derivs[16])
{
  // Coordinate conversion
  float r = 2.0*(pcoords[0]-0.5);
  float s = 2.0*(pcoords[1]-0.5);

  // Derivatives in the r-direction
  // midedge
  derivs[4] = -r * (1.0 - s);
  derivs[5] = 0.5 * (1.0 - s*s);
  derivs[6] = -r * (1.0 + s);
  derivs[7] = -0.5 * (1.0 - s*s);
  derivs[0] = -0.25 * (1.0 - s) - 0.5 * (derivs[7] + derivs[4]);
  derivs[1] =  0.25 * (1.0 - s) - 0.5 * (derivs[4] + derivs[5]);
  derivs[2] =  0.25 * (1.0 + s) - 0.5 * (derivs[5] + derivs[6]);
  derivs[3] = -0.25 * (1.0 + s) - 0.5 * (derivs[6] + derivs[7]);

  // Derivatives in the s-direction
  // midedge
  derivs[12] = -0.5 * (1.0 - r*r);
  derivs[13] = -s * (1.0 + r);
  derivs[14] = 0.5 * (1.0 - r*r);
  derivs[15] = -s * (1.0 - r);
  derivs[8] = -0.25 * (1.0 - r) - 0.5 * (derivs[15] + derivs[12]);
  derivs[9] = -0.25 * (1.0 + r) - 0.5 * (derivs[12] + derivs[13]);
  derivs[10] =  0.25 * (1.0 + r) - 0.5 * (derivs[13] + derivs[14]);
  derivs[11] =  0.25 * (1.0 - r) - 0.5 * (derivs[14] + derivs[15]);
}

