/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangleStrip.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkTriangleStrip.hh"
#include "vtkTriangle.hh"
#include "vtkCellArray.hh"
#include "vtkLine.hh"

// Description:
// Deep copy of cell.
vtkTriangleStrip::vtkTriangleStrip(const vtkTriangleStrip& ts)
{
  this->Points = ts.Points;
  this->PointIds = ts.PointIds;
}

int vtkTriangleStrip::EvaluatePosition(float x[3], float closestPoint[3],
                                      int& subId, float pcoords[3], 
                                      float& minDist2, float *weights)
{
  float pc[3], dist2;
  int ignoreId, i, return_status, status;
  float tempWeights[3], activeWeights[3];
  float closest[3];
  static vtkTriangle tri;

  pcoords[2] = 0.0;

  return_status = 0;
  for (minDist2=VTK_LARGE_FLOAT,i=0; i<this->Points.GetNumberOfPoints()-2; i++)
    {
    weights[i] = 0.0;
    tri.Points.SetPoint(0,this->Points.GetPoint(i));
    tri.Points.SetPoint(1,this->Points.GetPoint(i+1));
    tri.Points.SetPoint(2,this->Points.GetPoint(i+2));
    status = tri.EvaluatePosition(x,closest,ignoreId,pc,dist2,tempWeights);
    if ( status != -1 && dist2 < minDist2 )
      {
      return_status = status;
      closestPoint[0] = closest[0]; closestPoint[1] = closest[1]; closestPoint[2] = closest[2];
      subId = i;
      pcoords[0] = pc[0];
      pcoords[1] = pc[1];
      minDist2 = dist2;
      activeWeights[0] = tempWeights[0];
      activeWeights[1] = tempWeights[1];
      activeWeights[2] = tempWeights[2];
      }
    }

  weights[i] = 0.0;  
  weights[i+1] = 0.0;  

  weights[subId] = activeWeights[0];
  weights[subId+1] = activeWeights[1];
  weights[subId+2] = activeWeights[2];

  return return_status;
}

void vtkTriangleStrip::EvaluateLocation(int& subId, float pcoords[3], 
                                        float x[3], float *weights)
{
  int i;
  float *pt1 = this->Points.GetPoint(subId);
  float *pt2 = this->Points.GetPoint(subId+1);
  float *pt3 = this->Points.GetPoint(subId+2);
  float u3 = 1.0 - pcoords[0] - pcoords[1];

  for (i=0; i<3; i++)
    {
    x[i] = pt1[i]*pcoords[0] + pt2[i]*pcoords[1] + pt3[i]*u3;
    }

  weights[0] = u3;
  weights[1] = pcoords[0];
  weights[2] = pcoords[1];
}

int vtkTriangleStrip::CellBoundary(int subId, float pcoords[3], vtkIdList& pts)
{
  int numPts=this->Points.GetNumberOfPoints();
  int retStatus;
  static vtkTriangle tri;

  tri.Points.SetPoint(0,this->Points.GetPoint(subId));
  tri.Points.SetPoint(1,this->Points.GetPoint(subId+1));
  tri.Points.SetPoint(2,this->Points.GetPoint(subId+2));
  retStatus = tri.CellBoundary(0, pcoords, pts);

  if ( subId > 0 && subId < (numPts-3) ) //in the middle of the strip
    {
    pts.InsertId(0,this->PointIds.GetId(subId));
    pts.InsertId(1,this->PointIds.GetId(subId+2));
    }
  else if ( subId <= 0 ) //first triangle
    {
    pts.InsertId(0,this->PointIds.GetId(0));
    if ( pcoords[1] > pcoords[0] )
      {
      pts.InsertId(1,this->PointIds.GetId(1));
      }
    else
      {
      pts.InsertId(1,this->PointIds.GetId(2));
      }
    }
  else //last triangle
    {
    pts.InsertId(0,this->PointIds.GetId(numPts-1));
    if ( pcoords[0] < (1.0 - pcoords[0] - pcoords[1]) )
      {
      pts.InsertId(1,this->PointIds.GetId(numPts-3));
      }
    else
      {
      pts.InsertId(1,this->PointIds.GetId(numPts-2));
      }
    }
  return retStatus;

}

void vtkTriangleStrip::Contour(float value, vtkFloatScalars *cellScalars, 
                              vtkPointLocator *locator, vtkCellArray *verts, 
                              vtkCellArray *lines, vtkCellArray *polys, 
                              vtkFloatScalars *scalars)
{
  int i;
  vtkFloatScalars triScalars(3);
  static vtkTriangle tri;

  for ( i=0; i<this->Points.GetNumberOfPoints()-2; i++)
    {
    tri.Points.SetPoint(0,this->Points.GetPoint(i));
    tri.Points.SetPoint(1,this->Points.GetPoint(i+1));
    tri.Points.SetPoint(2,this->Points.GetPoint(i+2));

    triScalars.SetScalar(0,cellScalars->GetScalar(i));
    triScalars.SetScalar(1,cellScalars->GetScalar(i+1));
    triScalars.SetScalar(2,cellScalars->GetScalar(i+2));

    tri.Contour(value, &triScalars, locator, verts,
                lines, polys, scalars);
    }
}


vtkCell *vtkTriangleStrip::GetEdge(int edgeId)
{
  static vtkLine line;
  int id1, id2;

  if ( edgeId == 0 )
    {
    id1 = 0;
    id2 = 1;
    }
  else if ( edgeId == (this->GetNumberOfPoints()-1) )
    {
    id1 = edgeId - 1;
    id2 = edgeId;
    }
  else
    {
    id1 = edgeId - 1;
    id2 = edgeId + 1;
    }

  line.PointIds.SetId(0,this->PointIds.GetId(id1));
  line.PointIds.SetId(1,this->PointIds.GetId(id2));
  line.Points.SetPoint(0,this->Points.GetPoint(id1));
  line.Points.SetPoint(1,this->Points.GetPoint(id2));

  return &line;
}

// 
// Intersect sub-triangles
//
int vtkTriangleStrip::IntersectWithLine(float p1[3], float p2[3], float tol,
                                       float& t, float x[3], float pcoords[3],
                                       int& subId)
{
  static vtkTriangle tri;
  int subTest;

  for (subId=0; subId<this->Points.GetNumberOfPoints()-2; subId++)
    {
    tri.Points.SetPoint(0,this->Points.GetPoint(subId));
    tri.Points.SetPoint(1,this->Points.GetPoint(subId+1));
    tri.Points.SetPoint(2,this->Points.GetPoint(subId+2));

    if ( tri.IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      return 1;
    }

  return 0;
}

int vtkTriangleStrip::Triangulate(int vtkNotUsed(index), vtkFloatPoints &pts)
{
  pts.Reset();
  for (int subId=0; subId<this->Points.GetNumberOfPoints()-2; subId++)
    {
    pts.InsertNextPoint(this->Points.GetPoint(subId));
    pts.InsertNextPoint(this->Points.GetPoint(subId+1));
    pts.InsertNextPoint(this->Points.GetPoint(subId+2));
    }

  return 1;
}

void vtkTriangleStrip::Derivatives(int subId, float pcoords[3], float *values, 
                                   int dim, float *derivs)
{
  static vtkTriangle tri;

  tri.Points.SetPoint(0,this->Points.GetPoint(subId));
  tri.Points.SetPoint(1,this->Points.GetPoint(subId+1));
  tri.Points.SetPoint(2,this->Points.GetPoint(subId+2));

  tri.Derivatives(0, pcoords, values, dim, derivs);
}

// Description:
// Given a list of triangle strips, decompose into a list of (triangle) 
// polygons. The polygons are appended to the end of the list of polygons.
void vtkTriangleStrip::DecomposeStrips(vtkCellArray *strips, vtkCellArray *polys)
{
  int npts, *pts, p1, p2, p3, i;

  for (strips->InitTraversal(); strips->GetNextCell(npts,pts); )
    {
    p1 = pts[0];
    p2 = pts[1];
    p3 = pts[2];
    for (i=0; i<(npts-2); i++)
      {
      polys->InsertNextCell(3);
      if ( (i % 2) ) // flip ordering to preserve consistency
        {
        polys->InsertCellPoint(p2);
        polys->InsertCellPoint(p1);
        polys->InsertCellPoint(p3);
        }
      else
        {
        polys->InsertCellPoint(p1);
        polys->InsertCellPoint(p2);
        polys->InsertCellPoint(p3);
        }
      p1 = p2;
      p2 = p3;
      p3 = pts[3+i];
      }
    }
}
