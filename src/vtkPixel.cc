/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPixel.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkPixel.hh"
#include "vtkQuad.hh"
#include "vtkPolygon.hh"
#include "vtkPlane.hh"
#include "vtkMath.hh"
#include "vtkCellArray.hh"
#include "vtkLine.hh"

static vtkPlane vtkAPlane;

// Description:
// Deep copy of cell.
vtkPixel::vtkPixel(const vtkPixel& p)
{
  this->Points = p.Points;
  this->PointIds = p.PointIds;
}

int vtkPixel::EvaluatePosition(float x[3], float closestPoint[3],
                                  int& subId, float pcoords[3], 
                                  float& dist2, float *weights)
{
  float *pt1, *pt2, *pt3;
  int i;
  float p[3], p21[3], p31[3];
  float l21, l31, n[3];
  static vtkPolygon vtkAPoly;
  static vtkMath vtkAMath;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
//
// Get normal for pixel
//
  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);

  vtkAPoly.ComputeNormal (pt1, pt2, pt3, n);
//
// Project point to plane
//
  vtkAPlane.ProjectPoint(x,pt1,n,closestPoint);

  for (i=0; i<3; i++)
    {
    p21[i] = pt2[i] - pt1[i];
    p31[i] = pt3[i] - pt1[i];
    p[i] = x[i] - pt1[i];
    }

  if ( (l21=vtkAMath.Norm(p21)) == 0.0 ) l21 = 1.0;
  if ( (l31=vtkAMath.Norm(p31)) == 0.0 ) l31 = 1.0;

  pcoords[0] = vtkAMath.Dot(p21,p) / (l21*l21);
  pcoords[1] = vtkAMath.Dot(p31,p) / (l31*l31);

  this->InterpolationFunctions(pcoords, weights);

  if ( pcoords[0] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 )
    {
    dist2 = vtkAMath.Distance2BetweenPoints(closestPoint,x); //projection distance
    return 1;
    }
  else
    {
    float pc[3], w[4];
    for (i=0; i<2; i++)
      {
      if (pcoords[i] < 0.0) pc[i] = 0.0;
      else if (pcoords[i] > 1.0) pc[i] = 1.0;
      else pc[i] = pcoords[i];
      }
    this->EvaluateLocation(subId, pc, closestPoint, (float *)w);
    dist2 = vtkAMath.Distance2BetweenPoints(closestPoint,x);
    return 0;
    }
}

void vtkPixel::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                                   float *weights)
{
  float *pt1, *pt2, *pt3;
  int i;

  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i] + pcoords[0]*(pt2[i] - pt1[i]) +
                    pcoords[1]*(pt3[i] - pt1[i]);
    }

  this->InterpolationFunctions(pcoords, weights);
}

int vtkPixel::CellBoundary(int subId, float pcoords[3], vtkIdList& pts)
{
  float t1=pcoords[0]-pcoords[1];
  float t2=1.0-pcoords[0]-pcoords[1];

  pts.Reset();

  // compare against two lines in parametric space that divide element
  // into four pieces.
  if ( t1 >= 0.0 && t2 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(1));
    }

  else if ( t1 >= 0.0 && t2 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(1));
    pts.SetId(1,this->PointIds.GetId(3));
    }

  else if ( t1 < 0.0 && t2 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(3));
    pts.SetId(1,this->PointIds.GetId(2));
    }

  else //( t1 < 0.0 && t2 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(2));
    pts.SetId(1,this->PointIds.GetId(0));
    }

  if ( pcoords[0] < 0.0 || pcoords[0] > 1.0 ||
  pcoords[1] < 0.0 || pcoords[1] > 1.0 )
    return 0;
  else
    return 1;
}

//
// Marching squares
//
static int edges[4][2] = { {0,1}, {1,3}, {3,2}, {2,0} };

typedef int EDGE_LIST;
typedef struct {
       EDGE_LIST edges[5];
} LINE_CASES;

static LINE_CASES lineCases[] = { 
  {{-1, -1, -1, -1, -1}},
  {{0, 3, -1, -1, -1}},
  {{1, 0, -1, -1, -1}},
  {{1, 3, -1, -1, -1}},
  {{2, 1, -1, -1, -1}},
  {{0, 3, 2, 1, -1}},
  {{2, 0, -1, -1, -1}},
  {{2, 3, -1, -1, -1}},
  {{3, 2, -1, -1, -1}},
  {{0, 2, -1, -1, -1}},
  {{1, 0, 3, 2, -1}},
  {{1, 2, -1, -1, -1}},
  {{3, 1, -1, -1, -1}},
  {{0, 1, -1, -1, -1}},
  {{3, 0, -1, -1, -1}},
  {{-1, -1, -1, -1, -1}}
};

void vtkPixel::Contour(float value, vtkFloatScalars *cellScalars,
                     vtkFloatPoints *points, vtkCellArray *verts,
                     vtkCellArray *lines, vtkCellArray *polys, 
                     vtkFloatScalars *scalars)
{
  static int CASE_MASK[4] = {1,2,8,4}; //note difference!
  LINE_CASES *lineCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  int pts[2];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 4; i++)
      if (cellScalars->GetScalar(i) >= value)
          index |= CASE_MASK[i];

  lineCase = lineCases + index;
  edge = lineCase->edges;

  for ( ; edge[0] > -1; edge += 2 )
    {
    for (i=0; i<2; i++) // insert line
      {
      vert = edges[edge[i]];
      t = (value - cellScalars->GetScalar(vert[0])) /
          (cellScalars->GetScalar(vert[1]) - cellScalars->GetScalar(vert[0]));
      x1 = this->Points.GetPoint(vert[0]);
      x2 = this->Points.GetPoint(vert[1]);
      for (j=0; j<3; j++) x[j] = x1[j] + t * (x2[j] - x1[j]);
      pts[i] = points->InsertNextPoint(x);
      scalars->InsertNextScalar(value);
      }
    lines->InsertNextCell(2,pts);
    }
}

vtkCell *vtkPixel::GetEdge(int edgeId)
{
  static vtkLine line;
  int *verts;

  verts = edges[edgeId];

  // load point id's
  line.PointIds.SetId(0,this->PointIds.GetId(verts[0]));
  line.PointIds.SetId(1,this->PointIds.GetId(verts[1]));

  // load coordinates
  line.Points.SetPoint(0,this->Points.GetPoint(verts[0]));
  line.Points.SetPoint(1,this->Points.GetPoint(verts[1]));

  return &line;
}
//
// Compute interpolation functions (similar but different than Quad interpolation functions)
//
void vtkPixel::InterpolationFunctions(float pcoords[3], float sf[4])
{
  float rm, sm;

  rm = 1. - pcoords[0];
  sm = 1. - pcoords[1];

  sf[0] = rm * sm;
  sf[1] = pcoords[0] * sm;
  sf[2] = rm * pcoords[1];
  sf[3] = pcoords[0] * pcoords[1];
}

// 
// Intersect plane; see whether point is inside.
//
int vtkPixel::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                                float x[3], float pcoords[3], int& subId)
{
  float *pt1, *pt2, *pt3, *pt4, n[3];
  float tol2 = tol*tol;
  float closestPoint[3];
  float dist2, weights[4];
  int i;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
//
// Get normal for triangle
//
  pt1 = this->Points.GetPoint(0);
  pt2 = this->Points.GetPoint(1);
  pt3 = this->Points.GetPoint(2);
  pt4 = this->Points.GetPoint(3);

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
  if ( ! vtkAPlane.IntersectWithLine(p1,p2,n,pt1,t,x) ) return 0;
//
// Use evaluate position
//
  if (this->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights) )
    if ( dist2 <= tol2 ) return 1;

  return 0;
}

int vtkPixel::Triangulate(int index, vtkFloatPoints &pts)
{
  pts.Reset();
  pts.InsertPoint(0,this->Points.GetPoint(0));
  pts.InsertPoint(1,this->Points.GetPoint(1));
  pts.InsertPoint(2,this->Points.GetPoint(2));

  pts.InsertPoint(3,this->Points.GetPoint(1));
  pts.InsertPoint(4,this->Points.GetPoint(3));
  pts.InsertPoint(5,this->Points.GetPoint(2));

  return 1;
}

void vtkPixel::Derivatives(int subId, float pcoords[3], float *values, 
                            int dim, float *derivs)
{
  int i, idx;

  // The following code is incorrect. Will be fixed in future release.
  for (i=0; i<dim; i++)
    {
    idx = i*dim;
    derivs[idx] = 0.0;
    derivs[idx+1] = 0.0;
    derivs[idx+2] = 0.0;
    }
}

