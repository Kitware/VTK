/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Triangle.cc
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
#include "Triangle.hh"
#include "Polygon.hh"
#include "Plane.hh"
#include "vtkMath.hh"
#include "CellArr.hh"
#include "Line.hh"

static vtkPolygon poly;
static vtkMath math;
static vtkPlane plane;

// Description:
// Deep copy of cell.
vtkTriangle::vtkTriangle(const vtkTriangle& t)
{
  this->Points = t.Points;
  this->PointIds = t.PointIds;
}

int vtkTriangle::EvaluatePosition(float x[3], float closestPoint[3],
                                 int& subId, float pcoords[3], 
                                 float& dist2, float weights[MAX_CELL_SIZE])
{
  int i, j;
  float *pt1, *pt2, *pt3, n[3];
  float rhs[2], c1[2], c2[2];
  float det;
  float maxComponent;
  int idx, indices[2];
  int edge;

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
//
// Get normal for triangle
//
  pt1 = this->Points.GetPoint(1);
  pt2 = this->Points.GetPoint(2);
  pt3 = this->Points.GetPoint(0);

  poly.ComputeNormal (pt1, pt2, pt3, n);
//
// Project point to plane
//
  plane.ProjectPoint(x,pt1,n,closestPoint);
//
// Construct matrices.  Since we have over determined system, need to find
// which 2 out of 3 equations to use to develop equations. (Any 2 should 
// work since we've projected point to plane.)
//
  for (maxComponent=0.0, i=0; i<3; i++)
    {
    if (fabs(n[i]) > maxComponent)
      {
      maxComponent = fabs(n[i]);
      idx = i;
      }
    }
  for (j=0, i=0; i<3; i++)  
    {
    if ( i != idx ) indices[j++] = i;
    }
  
  for (i=0; i<2; i++)
    {  
    rhs[i] = closestPoint[indices[i]] - pt3[indices[i]];
    c1[i] = pt1[indices[i]] - pt3[indices[i]];
    c2[i] = pt2[indices[i]] - pt3[indices[i]];
    }

  if ( (det = math.Determinant2x2(c1,c2)) == 0.0 ) return -1;

  pcoords[0] = math.Determinant2x2 (rhs,c2) / det;
  pcoords[1] = math.Determinant2x2 (c1,rhs) / det;
  pcoords[2] = 1.0 - pcoords[0] - pcoords[1];
//
// Okay, now find closest point to element
//
  weights[0] = pcoords[2];
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];

  if ( pcoords[0] >= 0.0 && pcoords[0] <= 1.0 &&
  pcoords[1] >= 0.0 && pcoords[1] <= 1.0 &&
  pcoords[2] >= 0.0 && pcoords[2] <= 1.0 )
    {
    dist2 = math.Distance2BetweenPoints(closestPoint,x); //projection distance
    return 1;
    }
  else
    {
    static vtkLine line;
    float t;

    if ( pcoords[0] < 0.0 && pcoords[1] < 0.0 )
      {
      dist2 = math.Distance2BetweenPoints(x,pt3);
      for (i=0; i<3; i++) closestPoint[i] = pt3[i];
      }
    else if ( pcoords[1] < 0.0 && pcoords[2] < 0.0 )
      {
      dist2 = math.Distance2BetweenPoints(x,pt1);
      for (i=0; i<3; i++) closestPoint[i] = pt1[i];
      }
    else if ( pcoords[0] < 0.0 && pcoords[2] < 0.0 )
      {
      dist2 = math.Distance2BetweenPoints(x,pt2);
      for (i=0; i<3; i++) closestPoint[i] = pt2[i];
      }
    else if ( pcoords[0] < 0.0 )
      {
      dist2 = line.DistanceToLine(x,pt2,pt3,t,closestPoint);
      }
    else if ( pcoords[1] < 0.0 )
      {
      dist2 = line.DistanceToLine(x,pt1,pt3,t,closestPoint);
      }
    else if ( pcoords[2] < 0.0 )
      {
      dist2 = line.DistanceToLine(x,pt1,pt2,t,closestPoint);
      }
    return 0;
    }
}

void vtkTriangle::EvaluateLocation(int& subId, float pcoords[3], float x[3],
                                  float weights[MAX_CELL_SIZE])
{
  float u3;
  float *pt0, *pt1, *pt2;
  int i;

  pt0 = this->Points.GetPoint(0);
  pt1 = this->Points.GetPoint(1);
  pt2 = this->Points.GetPoint(2);

  u3 = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
    {
    x[i] = pt0[i]*u3 + pt1[i]*pcoords[0] + pt2[i]*pcoords[1];
    }

  weights[0] = u3;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
}

int vtkTriangle::CellBoundary(int subId, float pcoords[3], vtkIdList& pts)
{
  float t1=pcoords[0]-pcoords[1];
  float t2=0.5*(1.0-pcoords[0])-pcoords[1];
  float t3=2.0*pcoords[0]+pcoords[1]-1.0;

  pts.Reset();

  // compare against three lines in parametric space that divide element
  // into three pieces
  if ( t1 >= 0.0 && t2 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(0));
    pts.SetId(1,this->PointIds.GetId(1));
    }

  else if ( t2 < 0.0 && t3 >= 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(1));
    pts.SetId(1,this->PointIds.GetId(2));
    }

  else //( t1 < 0.0 && t3 < 0.0 )
    {
    pts.SetId(0,this->PointIds.GetId(2));
    pts.SetId(1,this->PointIds.GetId(0));
    }

  if ( pcoords[0] < 0.0 || pcoords[1] < 0.0 ||
  pcoords[0] > 1.0 || pcoords[1] > 1.0 || (1.0 - pcoords[0] - pcoords[1]) < 0.0 )
    return 0;
  else
    return 1;

}

//
// Marching triangles
//
typedef int EDGE_LIST;
typedef struct {
       EDGE_LIST edges[3];
} LINE_CASES;

static LINE_CASES lineCases[] = { 
  {-1, -1, -1},
  {0, 2, -1},
  {1, 0, -1},
  {1, 2, -1},
  {2, 1, -1},
  {0, 1, -1},
  {2, 0, -1},
  {-1, -1, -1}
};

void vtkTriangle::Contour(float value, vtkFloatScalars *cellScalars, 
                         vtkFloatPoints *points,
                         vtkCellArray *verts, vtkCellArray *lines, 
                         vtkCellArray *polys, vtkFloatScalars *scalars)
{
  static int CASE_MASK[3] = {1,2,4};
  LINE_CASES *lineCase;
  EDGE_LIST  *edge;
  int i, j, index, *vert;
  static int edges[3][2] = { {0,1}, {1,2}, {2,0} };
  int pts[2];
  float t, *x1, *x2, x[3];

  // Build the case table
  for ( i=0, index = 0; i < 3; i++)
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

vtkCell *vtkTriangle::GetEdge(int edgeId)
{
  static vtkLine line;
  int edgeIdPlus1 = edgeId + 1;

  if (edgeIdPlus1 > 2) edgeIdPlus1 = 0;

  // load point id's
  line.PointIds.SetId(0,this->PointIds.GetId(edgeId));
  line.PointIds.SetId(1,this->PointIds.GetId(edgeIdPlus1));

  // load coordinates
  line.Points.SetPoint(0,this->Points.GetPoint(edgeId));
  line.Points.SetPoint(1,this->Points.GetPoint(edgeIdPlus1));

  return &line;
}

//
// Plane intersection plus in/out test on triangle.
//
int vtkTriangle::IntersectWithLine(float p1[3], float p2[3], float tol, 
                                  float& t, float x[3], float pcoords[3], 
                                  int& subId)
{
  float *pt1, *pt2, *pt3, n[3];
  float tol2 = tol*tol;
  float closestPoint[3];
  float dist2, weights[MAX_CELL_SIZE];

  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
//
// Get normal for triangle
//
  pt1 = this->Points.GetPoint(1);
  pt2 = this->Points.GetPoint(2);
  pt3 = this->Points.GetPoint(0);

  poly.ComputeNormal (pt1, pt2, pt3, n);
//
// Intersect plane of triangle with line
//
  if ( ! plane.IntersectWithLine(p1,p2,n,pt1,t,x) ) return 0;
//
// Evaluate position
//
  if ( this->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights) )
    if ( dist2 <= tol2 ) return 1;

  return 0;
}

int vtkTriangle::Triangulate(int index, vtkFloatPoints &pts)
{
  pts.Reset();
  pts.InsertPoint(0,this->Points.GetPoint(0));
  pts.InsertPoint(1,this->Points.GetPoint(1));
  pts.InsertPoint(2,this->Points.GetPoint(2));

  return 1;
}

void vtkTriangle::Derivatives(int subId, float pcoords[3], float *values, 
                              int dim, float *derivs)
{

}

