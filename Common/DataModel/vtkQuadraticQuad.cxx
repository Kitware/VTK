/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticQuad.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadraticQuad.h"

#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkQuad.h"
#include "vtkQuadraticEdge.h"
#include "vtkPoints.h"

vtkStandardNewMacro(vtkQuadraticQuad);

//----------------------------------------------------------------------------
// Construct the quad with eight points.
vtkQuadraticQuad::vtkQuadraticQuad()
{
  this->Edge = vtkQuadraticEdge::New();
  this->Quad = vtkQuad::New();
  this->PointData = vtkPointData::New();
  this->CellData = vtkCellData::New();
  this->CellScalars = vtkDoubleArray::New();
  this->CellScalars->SetNumberOfTuples(9);
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(4);

  // We add a fictitious ninth point in order to process the cell. The ninth
  // point is in the center of the cell.
  this->Points->SetNumberOfPoints(9);
  this->PointIds->SetNumberOfIds(9);
  for (int i = 0; i < 9; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }
  this->Points->SetNumberOfPoints(8);
  this->PointIds->SetNumberOfIds(8);
}

//----------------------------------------------------------------------------
vtkQuadraticQuad::~vtkQuadraticQuad()
{
  this->Edge->Delete();
  this->Quad->Delete();

  this->Scalars->Delete();
  this->PointData->Delete();
  this->CellData->Delete();
  this->CellScalars->Delete();
}
//----------------------------------------------------------------------------
vtkCell *vtkQuadraticQuad::GetEdge(int edgeId)
{
  edgeId = (edgeId < 0 ? 0 : (edgeId > 3 ? 3 : edgeId ));
  int p = (edgeId+1) % 4;

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

//----------------------------------------------------------------------------

static int LinearQuads[4][4] = { {0, 4, 8, 7}, {8, 4, 1, 5},
                                 {8, 5, 2, 6}, {7, 8, 6, 3} };

void vtkQuadraticQuad::Subdivide(double *weights)
{
  int i, j;
  double pc[3], x[3];

  pc[0] = pc[1] = 0.5;
  this->InterpolationFunctions(pc, weights);

  double p[3];
  x[0] = x[1] = x[2] = 0.0;
  for (i=0; i<8; i++)
    {
    this->Points->GetPoint(i, p);
    for (j=0; j<3; j++)
      {
      x[j] += p[j] * weights[i];
      }
    }
  this->Points->SetPoint(8,x);
}

//----------------------------------------------------------------------------
int vtkQuadraticQuad::EvaluatePosition(double* x,
                                       double* closestPoint,
                                       int& subId, double pcoords[3],
                                       double& minDist2, double *weights)
{
  double pc[3], dist2;
  int ignoreId, i, returnStatus=0, status;
  double tempWeights[4];
  double closest[3];

  // compute the midquad node
  this->Subdivide(weights);

  //four linear quads are used
  for (minDist2=VTK_DOUBLE_MAX, i=0; i < 4; i++)
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
    if(closestPoint!=0)
      {
      // Compute both closestPoint and weights
      this->EvaluateLocation(subId,pcoords,closestPoint,weights);
      }
    else
      {
      // Compute weigths only
      this->InterpolationFunctions(pcoords,weights);
      }
    }

  return returnStatus;
}
//----------------------------------------------------------------------------
void vtkQuadraticQuad::EvaluateLocation(int& vtkNotUsed(subId),
                                        double pcoords[3],
                                        double x[3], double *weights)
{
  int i, j;
  double *p =
    static_cast<vtkDoubleArray *>(this->Points->GetData())->GetPointer(0);

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

//----------------------------------------------------------------------------
int vtkQuadraticQuad::CellBoundary(int subId, double pcoords[3], vtkIdList *pts)
{
  return this->Quad->CellBoundary(subId, pcoords, pts);
}

static double MidPoints[1][3] = { {0.5,0.5,0.0} };

//----------------------------------------------------------------------------
void vtkQuadraticQuad::InterpolateAttributes(vtkPointData *inPd, vtkCellData *inCd,
                                             vtkIdType cellId, vtkDataArray *cellScalars)
{
  int numMidPts, i, j;
  double weights[20];
  double x[3];
  double s;

  //Copy point and cell attribute data, first make sure it's empty:
  this->PointData->Initialize();
  this->CellData->Initialize();
  // Make sure to copy ALL arrays. These field data have to be
  // identical to the input field data. Otherwise, CopyData
  // that occurs later may not work because the output field
  // data was initialized (CopyAllocate) with the input field
  // data.
  this->PointData->CopyAllOn();
  this->CellData->CopyAllOn();
  this->PointData->CopyAllocate(inPd,9);
  this->CellData->CopyAllocate(inCd,4);

  // copy the point data over into point ids 0->7
  for (i=0; i<8; i++)
    {
    this->PointData->CopyData(inPd,this->PointIds->GetId(i),i);
    this->CellScalars->SetValue( i, cellScalars->GetTuple1(i));
    }
  // copy the cell data over to the linear cell
  this->CellData->CopyData(inCd,cellId,0);

  //Interpolate new values
  double p[3];
  this->Points->Resize(9);
  this->CellScalars->Resize(9);
  for ( numMidPts=0; numMidPts < 1; numMidPts++ )
    {
    this->InterpolationFunctions(MidPoints[numMidPts], weights);

    x[0] = x[1] = x[2] = 0.0;
    s = 0.0;
    for (i=0; i<8; i++)
      {
      this->Points->GetPoint(i, p);
      for (j=0; j<3; j++)
        {
        x[j] += p[j] * weights[i];
        }
      s += cellScalars->GetTuple1(i) * weights[i];
      }
    this->Points->SetPoint(8+numMidPts,x);
    this->CellScalars->SetValue(8+numMidPts,s);
    this->PointData->InterpolatePoint(inPd, 8+numMidPts,
                                      this->PointIds, weights);
    }
}

//----------------------------------------------------------------------------
void vtkQuadraticQuad::Contour(double value,
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
  //interpolate point and cell data
  this->InterpolateAttributes(inPd,inCd,cellId,cellScalars);

  //contour each linear quad separately
  for (int i=0; i<4; i++)
    {
    for (int j=0; j<4; j++) //for each of the four vertices of the linear quad
      {
      this->Quad->Points->SetPoint(j,this->Points->GetPoint(LinearQuads[i][j]));
      this->Quad->PointIds->SetId(j,LinearQuads[i][j]);
      this->Scalars->SetValue(j,this->CellScalars->GetValue(LinearQuads[i][j]));
      }

    this->Quad->Contour(value,this->Scalars,locator,verts,lines,polys,
                        this->PointData,outPd,this->CellData,cellId,outCd);
    }
}
//----------------------------------------------------------------------------
// Clip this quadratic quad using scalar value provided. Like contouring,
// except that it cuts the quad to produce other quads and triangles.
void vtkQuadraticQuad::Clip(double value, vtkDataArray* cellScalars,
                            vtkIncrementalPointLocator* locator, vtkCellArray* polys,
                            vtkPointData* inPd, vtkPointData* outPd,
                            vtkCellData* inCd, vtkIdType cellId,
                            vtkCellData* outCd, int insideOut)
{
  //interpolate point and cell data
  this->InterpolateAttributes(inPd,inCd,cellId,cellScalars);

  //contour each linear quad separately
  for (int i=0; i<4; i++)
    {
    for ( int j=0; j<4; j++) //for each of the four vertices of the linear quad
      {
      this->Quad->Points->SetPoint(j,this->Points->GetPoint(LinearQuads[i][j]));
      this->Quad->PointIds->SetId(j,LinearQuads[i][j]);
      this->Scalars->SetValue(j,this->CellScalars->GetValue(LinearQuads[i][j]));
      }

    this->Quad->Clip(value,this->Scalars,locator,polys,this->PointData,
                     outPd,this->CellData,cellId,outCd,insideOut);
    }
}


//----------------------------------------------------------------------------
// Line-line intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkQuadraticQuad::IntersectWithLine(double* p1,
                                        double* p2,
                                        double tol,
                                        double& t,
                                        double* x,
                                        double* pcoords,
                                        int& subId)
{
  int subTest, i;
  subId = 0;
  double weights[8];

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

//----------------------------------------------------------------------------
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
  double x4[3], x5[3], x6[3], x7[3];
  this->Points->GetPoint(4, x4);
  this->Points->GetPoint(5, x5);
  this->Points->GetPoint(6, x6);
  this->Points->GetPoint(7, x7);

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

//----------------------------------------------------------------------------
void vtkQuadraticQuad::Derivatives(int vtkNotUsed(subId),
                                   double pcoords[3], double *values,
                                   int dim, double *derivs)
{
  double x0[3], x1[3], x2[3], deltaX[3], weights[8];
  int i, j;
  double functionDerivs[16];

  this->Points->GetPoint(0, x0);
  this->Points->GetPoint(1, x1);
  this->Points->GetPoint(2, x2);

  this->InterpolationFunctions(pcoords,weights);
  this->InterpolationDerivs(pcoords,functionDerivs);

  for (i=0; i<3; i++)
    {
    deltaX[i] = x1[i] - x0[i] - x2[i];
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
        derivs[3*i+j] = 0;
        }
      }
    }
}

//----------------------------------------------------------------------------
// Compute interpolation functions. The first four nodes are the corner
// vertices; the others are mid-edge nodes.
void vtkQuadraticQuad::InterpolationFunctions(double pcoords[3],
                                              double weights[8])
{
  //VTK needs parametric coordinates to be between (0,1). Isoparametric
  //shape functions are formulated between (-1,1). Here we do a
  //coordinate system conversion from (0,1) to (-1,1).
  double r = pcoords[0];
  double s = pcoords[1];

  //midedge weights
  weights[4] = 4 * r * (1.0 - r) * (1.0 - s);
  weights[5] = 4 * r             * (1.0 - s) * s;
  weights[6] = 4 * r * (1.0 - r) *             s;
  weights[7] = 4 *     (1.0 - r) * (1.0 - s) * s;

  //corner
  weights[0] = (1.0 - r) * (1.0 - s) - 0.5*(weights[4]+weights[7]);
  weights[1] = r * (1.0 - s) - 0.5*(weights[4]+weights[5]);
  weights[2] = r * s - 0.5*(weights[5]+weights[6]);
  weights[3] = (1.0 - r) * s - 0.5*(weights[6]+weights[7]);

}

//----------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkQuadraticQuad::InterpolationDerivs(double pcoords[3], double derivs[16])
{
  // Coordinate conversion
  double r = pcoords[0];
  double s = pcoords[1];

  // Derivatives in the r-direction
  // midedge
  derivs[4] = 4 * (1.0 - s) * (1.0 - 2*r);
  derivs[5] = 4 * (1.0 - s) * s;
  derivs[6] = 4 * s * (1.0 - 2*r);
  derivs[7] =-4 * (1.0 - s) * s;
  derivs[0] =-(1.0 - s) - 0.5 * (derivs[4] + derivs[7]);
  derivs[1] = (1.0 - s) - 0.5 * (derivs[4] + derivs[5]);
  derivs[2] =        s  - 0.5 * (derivs[5] + derivs[6]);
  derivs[3] =       -s  - 0.5 * (derivs[6] + derivs[7]);

  // Derivatives in the s-direction
  // midedge
  derivs[12] =-4 * r * (1.0 - r);
  derivs[13] = 4 * r * (1.0 - 2*s);
  derivs[14] = 4 * r * (1.0 - r);
  derivs[15] = 4 * (1.0 - r) * (1.0 - 2*s);
  derivs[8] = -(1.0 - r) - 0.5 * (derivs[12] + derivs[15]);
  derivs[9] =       - r  - 0.5 * (derivs[12] + derivs[13]);
  derivs[10] =        r  - 0.5 * (derivs[13] + derivs[14]);
  derivs[11] = (1.0 - r) - 0.5 * (derivs[14] + derivs[15]);
}

//----------------------------------------------------------------------------
static double vtkQQuadCellPCoords[24] = {0.0,0.0,0.0, 1.0,0.0,0.0,
                                        1.0,1.0,0.0, 0.0,1.0,0.0,
                                        0.5,0.0,0.0, 1.0,0.5,0.0,
                                        0.5,1.0,0.0, 0.0,0.5,0.0};
double *vtkQuadraticQuad::GetParametricCoords()
{
  return vtkQQuadCellPCoords;
}

//----------------------------------------------------------------------------
void vtkQuadraticQuad::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Quad:\n";
  this->Quad->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Scalars:\n";
  this->Scalars->PrintSelf(os,indent.GetNextIndent());
}
