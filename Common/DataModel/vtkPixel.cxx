/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPixel.h"

#include "vtkObjectFactory.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkPlane.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkLine.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPoints.h"
#include "vtkMarchingSquaresLineCases.h"

vtkStandardNewMacro(vtkPixel);

//----------------------------------------------------------------------------
// Construct the pixel with four points.
vtkPixel::vtkPixel()
{
  int i;

  this->Points->SetNumberOfPoints(4);
  this->PointIds->SetNumberOfIds(4);
  for (i = 0; i < 4; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
  }
  for (i = 0; i < 4; i++)
  {
    this->PointIds->SetId(i,0);
  }
  this->Line = vtkLine::New();
}

//----------------------------------------------------------------------------
vtkPixel::~vtkPixel()
{
  this->Line->Delete();
}

//----------------------------------------------------------------------------
int vtkPixel::EvaluatePosition(double x[3], double* closestPoint,
                                  int& subId, double pcoords[3],
                                  double& dist2, double *weights)
{
  double pt1[3], pt2[3], pt3[3];
  int i;
  double p[3], p21[3], p31[3], cp[3];
  double l21, l31, n[3];

  subId = 0;
  pcoords[2] = 0.0;

  // Get normal for pixel
  //
  this->Points->GetPoint(0, pt1);
  this->Points->GetPoint(1, pt2);
  this->Points->GetPoint(2, pt3);

  vtkTriangle::ComputeNormal (pt1, pt2, pt3, n);

  // Project point to plane
  //
  vtkPlane::ProjectPoint(x,pt1,n,cp);

  for (i=0; i<3; i++)
  {
    p21[i] = pt2[i] - pt1[i];
    p31[i] = pt3[i] - pt1[i];
    p[i] = x[i] - pt1[i];
  }

  if ( (l21=vtkMath::Norm(p21)) == 0.0 )
  {
    l21 = 1.0;
  }
  if ( (l31=vtkMath::Norm(p31)) == 0.0 )
  {
    l31 = 1.0;
  }

  pcoords[0] = vtkMath::Dot(p21,p) / (l21*l21);
  pcoords[1] = vtkMath::Dot(p31,p) / (l31*l31);

  this->InterpolationFunctions(pcoords, weights);

  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 )
  {
    if (closestPoint)
    {
      closestPoint[0] = cp[0];
      closestPoint[1] = cp[1];
      closestPoint[2] = cp[2];
      dist2 =
        vtkMath::Distance2BetweenPoints(closestPoint,x); //projection distance
    }
    return 1;
  }
  else
  {
    double pc[3], w[4];
    if (closestPoint)
    {
      for (i=0; i<2; i++)
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
void vtkPixel::EvaluateLocation(int& subId, double pcoords[3], double x[3],
                                   double *weights)
{
  double pt1[3], pt2[3], pt3[3];
  int i;

  subId = 0;

  this->Points->GetPoint(0, pt1);
  this->Points->GetPoint(1, pt2);
  this->Points->GetPoint(2, pt3);

  for (i=0; i<3; i++)
  {
    x[i] = pt1[i] + pcoords[0]*(pt2[i] - pt1[i]) +
                    pcoords[1]*(pt3[i] - pt1[i]);
  }

  this->InterpolationFunctions(pcoords, weights);
}

//----------------------------------------------------------------------------
int vtkPixel::CellBoundary(int vtkNotUsed(subId), double pcoords[3], vtkIdList *pts)
{
  double t1=pcoords[0]-pcoords[1];
  double t2=1.0-pcoords[0]-pcoords[1];

  pts->SetNumberOfIds(2);

  // compare against two lines in parametric space that divide element
  // into four pieces.
  if ( t1 >= 0.0 && t2 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(0));
    pts->SetId(1,this->PointIds->GetId(1));
  }

  else if ( t1 >= 0.0 && t2 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(1));
    pts->SetId(1,this->PointIds->GetId(3));
  }

  else if ( t1 < 0.0 && t2 < 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(3));
    pts->SetId(1,this->PointIds->GetId(2));
  }

  else //( t1 < 0.0 && t2 >= 0.0 )
  {
    pts->SetId(0,this->PointIds->GetId(2));
    pts->SetId(1,this->PointIds->GetId(0));
  }

  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
       pcoords[1] < 0.0 || pcoords[1] > 1.0 )
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//----------------------------------------------------------------------------
//
// Marching squares
//
#include "vtkMarchingSquaresLineCases.h"

static int edges[4][2] = { {0,1}, {1,3}, {2,3}, {0,2} };

void vtkPixel::Contour(double value, vtkDataArray *cellScalars,
                       vtkIncrementalPointLocator *locator,
                       vtkCellArray *vtkNotUsed(verts),
                       vtkCellArray *lines,
                       vtkCellArray *vtkNotUsed(polys),
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd)
{
  static int CASE_MASK[4] = {1,2,8,4}; //note differenceom quad!
  vtkMarchingSquaresLineCases *lineCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  int newCellId;
  vtkIdType pts[2];
  double t, x1[3], x2[3], x[3];

  // Build the case table
  for ( i=0, index = 0; i < 4; i++)
  {
    if (cellScalars->GetComponent(i,0) >= value)
    {
      index |= CASE_MASK[i];
    }
  }

  lineCase = vtkMarchingSquaresLineCases::GetCases() + index;
  edge = lineCase->edges;

  for ( ; edge[0] > -1; edge += 2 )
  {
    for (i=0; i<2; i++) // insert line
    {
      vert = edges[edge[i]];
      t = (value - cellScalars->GetComponent(vert[0],0)) /
          (cellScalars->GetComponent(vert[1],0) -
           cellScalars->GetComponent(vert[0],0));
      this->Points->GetPoint(vert[0], x1);
      this->Points->GetPoint(vert[1], x2);
      for (j=0; j<3; j++)
      {
        x[j] = x1[j] + t * (x2[j] - x1[j]);
      }
      if ( locator->InsertUniquePoint(x, pts[i]) )
      {
        if ( outPd )
        {
          int p1 = this->PointIds->GetId(vert[0]);
          int p2 = this->PointIds->GetId(vert[1]);
          outPd->InterpolateEdge(inPd,pts[i],p1,p2,t);
        }
      }
    }
    // check for degenerate line
    if ( pts[0] != pts[1] )
    {
      newCellId = lines->InsertNextCell(2,pts);
      outCd->CopyData(inCd,cellId,newCellId);
    }
  }
}

//----------------------------------------------------------------------------
vtkCell *vtkPixel::GetEdge(int edgeId)
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
//
// Compute interpolation functions (similar but different than Quad interpolation
// functions)
//
void vtkPixel::InterpolationFunctions(double pcoords[3], double sf[4])
{
  double rm, sm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];

  sf[0] = rm * sm;
  sf[1] = pcoords[0] * sm;
  sf[2] = rm * pcoords[1];
  sf[3] = pcoords[0] * pcoords[1];
}
//----------------------------------------------------------------------------
//
// Compute derivatives of interpolation functions.
//
void vtkPixel::InterpolationDerivs(double pcoords[3], double derivs[8])
{
  double rm, sm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];

  // r derivatives
  derivs[0] = -sm;
  derivs[1] = sm;
  derivs[2] = -pcoords[1];
  derivs[3] = pcoords[1];

  // s derivatives
  derivs[4] = -rm;
  derivs[5] = -pcoords[0];
  derivs[6] = rm;
  derivs[7] = pcoords[0];
}

//----------------------------------------------------------------------------
//
// Intersect plane; see whether point is inside.
//
int vtkPixel::IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                                double x[3], double pcoords[3], int& subId)
{
  double pt1[3], pt4[3], n[3];
  double tol2 = tol*tol;
  double closestPoint[3];
  double dist2, weights[4];
  int i;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
//
// Get normal for triangle
//
  this->Points->GetPoint(0, pt1);
  this->Points->GetPoint(3, pt4);

  n[0] = n[1] = n[2] = 0.0;
  for (i=0; i<3; i++)
  {
    if ( (pt4[i] - pt1[i]) <= 0.0 )
    {
      n[i] = 1.0;
      break;
    }
  }
  //
  // Intersect plane of pixel with line
  //
  if ( ! vtkPlane::IntersectWithLine(p1,p2,n,pt1,t,x) )
  {
    return 0;
  }
  //
  // Use evaluate position
  //
  if (this->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights))
  {
    if ( dist2 <= tol2 )
    {
      return 1;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkPixel::Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  if ( (index % 2) )
  {
    ptIds->InsertId(0,this->PointIds->GetId(0));
    pts->InsertPoint(0,this->Points->GetPoint(0));
    ptIds->InsertId(1,this->PointIds->GetId(1));
    pts->InsertPoint(1,this->Points->GetPoint(1));
    ptIds->InsertId(2,this->PointIds->GetId(2));
    pts->InsertPoint(2,this->Points->GetPoint(2));

    ptIds->InsertId(3,this->PointIds->GetId(1));
    pts->InsertPoint(3,this->Points->GetPoint(1));
    ptIds->InsertId(4,this->PointIds->GetId(3));
    pts->InsertPoint(4,this->Points->GetPoint(3));
    ptIds->InsertId(5,this->PointIds->GetId(2));
    pts->InsertPoint(5,this->Points->GetPoint(2));
  }
  else
  {
    ptIds->InsertId(0,this->PointIds->GetId(0));
    pts->InsertPoint(0,this->Points->GetPoint(0));
    ptIds->InsertId(1,this->PointIds->GetId(1));
    pts->InsertPoint(1,this->Points->GetPoint(1));
    ptIds->InsertId(2,this->PointIds->GetId(3));
    pts->InsertPoint(2,this->Points->GetPoint(3));

    ptIds->InsertId(3,this->PointIds->GetId(0));
    pts->InsertPoint(3,this->Points->GetPoint(0));
    ptIds->InsertId(4,this->PointIds->GetId(3));
    pts->InsertPoint(4,this->Points->GetPoint(3));
    ptIds->InsertId(5,this->PointIds->GetId(2));
    pts->InsertPoint(5,this->Points->GetPoint(2));
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPixel::Derivatives(int vtkNotUsed(subId),
                           double pcoords[3],
                           double *values,
                           int dim, double *derivs)
{
  double functionDerivs[8], sum;
  int i, j, k, plane, idx[2], jj;
  double x0[3], x1[3], x2[3], x3[3], spacing[3];

  this->Points->GetPoint(0, x0);
  this->Points->GetPoint(1, x1);
  this->Points->GetPoint(2, x2);
  this->Points->GetPoint(3, x3);

  //figure which plane this pixel is in
  for (i=0; i < 3; i++)
  {
    spacing[i] = x3[i] - x0[i];
  }

  if ( spacing[0] > spacing[2] && spacing[1] > spacing[2] ) // z-plane
  {
    plane = 2;
    idx[0] = 0; idx[1] = 1;
  }
  else if ( spacing[0] > spacing[1] && spacing[2] > spacing[1] ) // y-plane
  {
    plane = 1;
    idx[0] = 0; idx[1] = 2;
  }
  else // x-plane
  {
    plane = 0;
    idx[0] = 1; idx[1] = 2;
  }

  spacing[0] = x1[idx[0]] - x0[idx[0]];
  spacing[1] = x2[idx[1]] - x0[idx[1]];

  // get derivatives in r-s directions
  this->InterpolationDerivs(pcoords, functionDerivs);

  // since two of the x-y-z axes are aligned with r-s axes, only need to scale
  // the derivative values by the data spacing.
  for (k=0; k < dim; k++) //loop over values per vertex
  {
    for (jj=j=0; j < 3; j++) //loop over derivative directions
    {
      if ( j == plane ) // 0-derivate values in this direction
      {
        sum = 0.0;
      }
      else //compute derivatives
      {
        for (sum=0.0, i=0; i < 4; i++) //loop over interp. function derivatives
        {
          sum += functionDerivs[4*jj + i] * values[dim*i + k];
        }
        sum /= spacing[idx[jj++]];
      }
      derivs[3*k + j] = sum;
    }
  }
}

//----------------------------------------------------------------------------
// support pixel clipping
typedef int PIXEL_EDGE_LIST;
typedef struct {
       PIXEL_EDGE_LIST edges[14];
} PIXEL_CASES;

static PIXEL_CASES pixelCases[] = {
{{  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 0
{{   3, 100,   0,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 1
{{   3, 101,   1,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 2
{{   4, 100, 101,   1,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 3
{{   3, 103,   2,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 4
{{   3, 100,   0,   3,   3, 103,   2,   1,   4,   0,   1,   2,   3,  -1}}, // 5
{{   4, 101, 103,   2,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 6
{{   3, 100, 101,   3,   3, 101,   2,   3,   3, 101, 103,   2,  -1,  -1}}, // 7
{{   3, 102,   3,   2,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 8
{{   4, 100,   0,   2, 102,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 9
{{   3, 101,   1,   0,   3, 102,   3,   2,   4,   0,   1,   2,   3,  -1}}, // 10
{{   3, 100, 101,   1,   3, 100,   1,   2,   3, 100,   2, 102,  -1,  -1}}, // 11
{{   4, 103, 102,   3,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 12
{{   3, 100,   0, 102,   3,   0,   1, 102,   3,   1, 103, 102,  -1,  -1}}, // 13
{{   3,   0, 101, 103,   3,   0, 103,   3,   3, 103, 102,   3,  -1,  -1}}, // 14
{{   4, 100, 101, 103, 102,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 15
};

static PIXEL_CASES pixelCasesComplement[] = {
{{  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 0
{{   3, 100,   0,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 1
{{   3, 101,   1,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 2
{{   4, 100, 101,   1,   3,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 3
{{   3, 103,   2,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 4
{{   3, 100,   0,   3,   3, 103,   2,   1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 5
{{   4, 101, 103,   2,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 6
{{   3, 100, 101,   3,   3, 101,   2,   3,   3, 101, 103,   2,  -1,  -1}}, // 7
{{   3, 102,   3,   2,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 8
{{   4, 100,   0,   2, 102,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 9
{{   3, 101,   1,   0,   3, 102,   3,   2,  -1,  -1,  -1,  -1,  -1,  -1}}, // 10
{{   3, 100, 101,   1,   3, 100,   1,   2,   3, 100,   2, 102,  -1,  -1}}, // 11
{{   4, 103, 102,   3,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 12
{{   3, 100,   0, 102,   3,   0,   1, 102,   3,   1, 103, 102,  -1,  -1}}, // 13
{{   3,   0, 101, 103,   3,   0, 103,   3,   3, 103, 102,   3,  -1,  -1}}, // 14
{{   4, 100, 101, 103, 102,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}}, // 15
};


//----------------------------------------------------------------------------
// Clip this pixel using scalar value provided. Like contouring, except
// that it cuts the pixel to produce quads and/or triangles.
void vtkPixel::Clip(double value, vtkDataArray *cellScalars,
                   vtkIncrementalPointLocator *locator, vtkCellArray *polys,
                   vtkPointData *inPd, vtkPointData *outPd,
                   vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                   int insideOut)
{
  static int CASE_MASK[4] = {1,2,8,4}; //note difference from quad!
  PIXEL_CASES *pixelCase;
  PIXEL_EDGE_LIST  *edge;
  int i, j, index, *vert;
  int e1, e2;
  int newCellId;
  vtkIdType pts[4];
  int vertexId;
  double t, x1[3], x2[3], x[3], deltaScalar;
  double scalar0, scalar1, e1Scalar;

  // Build the index into the case table
  if ( insideOut )
  {
    for ( i=0, index = 0; i < 4; i++)
    {
      if (cellScalars->GetComponent(i,0) <= value)
      {
        index |= CASE_MASK[i];
      }
    }
    // Select case based on the index and get the list of edges for this case
    pixelCase = pixelCases + index;
  }
  else
  {
    for ( i=0, index = 0; i < 4; i++)
    {
      if (cellScalars->GetComponent(i,0) > value)
      {
        index |= CASE_MASK[i];
      }
    }
    // Select case based on the index and get the list of edges for this case
    pixelCase = pixelCasesComplement + index;
  }

  edge = pixelCase->edges;

  // generate each pixel
  for ( ; edge[0] > -1; edge += edge[0]+1 )
  {
    for (i=0; i < edge[0]; i++) // insert pixel or triangle
    {
      // vertex exists, and need not be interpolated
      if (edge[i+1] >= 100)
      {
        vertexId = edge[i+1] - 100;
        this->Points->GetPoint(vertexId, x);
        if ( locator->InsertUniquePoint(x, pts[i]) )
        {
          outPd->CopyData(inPd,this->PointIds->GetId(vertexId),pts[i]);
        }
      }

      else //new vertex, interpolate
      {
        vert = edges[edge[i+1]];

        // calculate a preferred interpolation direction
        scalar0 = cellScalars->GetComponent(vert[0],0);
        scalar1 = cellScalars->GetComponent(vert[1],0);
        deltaScalar = scalar1 - scalar0;

        if (deltaScalar > 0)
        {
          e1 = vert[0]; e2 = vert[1];
          e1Scalar = scalar0;
        }
        else
        {
          e1 = vert[1]; e2 = vert[0];
          e1Scalar = scalar1;
          deltaScalar = -deltaScalar;
        }

        // linear interpolation
        if (deltaScalar == 0.0)
        {
          t = 0.0;
        }
        else
        {
          t = (value - e1Scalar) / deltaScalar;
        }

        this->Points->GetPoint(e1, x1);
        this->Points->GetPoint(e2, x2);

        for (j=0; j<3; j++)
        {
          x[j] = x1[j] + t * (x2[j] - x1[j]);
        }

        if ( locator->InsertUniquePoint(x, pts[i]) )
        {
          int p1 = this->PointIds->GetId(e1);
          int p2 = this->PointIds->GetId(e2);
          outPd->InterpolateEdge(inPd,pts[i],p1,p2,t);
        }
      }
    }
    // check for degenerate output
    if ( edge[0] == 3 ) //i.e., a triangle
    {
      if (pts[0] == pts[1] || pts[0] == pts[2] || pts[1] == pts[2] )
      {
        continue;
      }
    }
    else // a pixel
    {
      if ((pts[0] == pts[3] && pts[1] == pts[2]) ||
          (pts[0] == pts[1] && pts[3] == pts[2]) )
      {
        continue;
      }
    }

    newCellId = polys->InsertNextCell(edge[0],pts);
    outCd->CopyData(inCd,cellId,newCellId);
  }
}

//----------------------------------------------------------------------------
static double vtkPixelCellPCoords[12] = {0.0,0.0,0.0, 1.0,0.0,0.0,
                                        0.0,1.0,0.0, 1.0,1.0,0.0};

double *vtkPixel::GetParametricCoords()
{
  return vtkPixelCellPCoords;
}

//----------------------------------------------------------------------------
void vtkPixel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os,indent.GetNextIndent());
}

