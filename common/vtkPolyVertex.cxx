/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyVertex.cxx
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
#include "vtkPolyVertex.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkVertex.h"
#include "vtkPointLocator.h"

// Description:
// Deep copy of cell.
vtkPolyVertex::vtkPolyVertex(const vtkPolyVertex& pp)
{
  this->Points = pp.Points;
  this->PointIds = pp.PointIds;
}

int vtkPolyVertex::EvaluatePosition(float x[3], float closestPoint[3],
                                   int& subId, float pcoords[3], 
                                   float& minDist2, float *weights)
{
  int numPts=this->Points.GetNumberOfPoints();
  float *X;
  float dist2;
  int i;

  for (minDist2=VTK_LARGE_FLOAT, i=0; i<numPts; i++)
    {
    X = this->Points.GetPoint(i);
    dist2 = vtkMath::Distance2BetweenPoints(X,x);
    if (dist2 < minDist2)
      {
      closestPoint[0] = X[0]; closestPoint[1] = X[1]; closestPoint[2] = X[2];
      minDist2 = dist2;
      subId = i;
      }
    }

  for (i=0; i<numPts; i++) weights[i] = 0.0;
  weights[subId] = 1.0;

  if (minDist2 == 0.0)
    {
    pcoords[0] = 0.0;
    return 1;
    }
  else
    {
    pcoords[0] = -10.0;
    return 0;
    }

}

void vtkPolyVertex::EvaluateLocation(int& subId, 
				     float vtkNotUsed(pcoords)[3], 
                                     float x[3], float *weights)
{
  int i;
  float *X = this->Points.GetPoint(subId);
  x[0] = X[0];
  x[1] = X[1];
  x[2] = X[2];

  for (i=0; i<this->GetNumberOfPoints(); i++) weights[i] = 0.0;
  weights[subId] = 1.0;
}

int vtkPolyVertex::CellBoundary(int subId, float pcoords[3], vtkIdList& pts)
{
  pts.Reset();
  pts.SetId(subId,this->PointIds.GetId(subId));

  if ( pcoords[0] != 0.0 )  
    return 0;
  else
    return 1;

}

void vtkPolyVertex::Contour(float value, vtkFloatScalars *cellScalars, 
			    vtkPointLocator *locator, vtkCellArray *verts,
			    vtkCellArray *vtkNotUsed(lines), 
			    vtkCellArray *vtkNotUsed(polys), 
                            vtkPointData *inPd, vtkPointData *outPd)
{
  int i, pts[1], numPts=this->Points.GetNumberOfPoints();

  for (i=0; i < numPts; i++)
    {
    if ( value == cellScalars->GetScalar(i) )
      {
      pts[0] = locator->InsertNextPoint(this->Points.GetPoint(i));
      if ( outPd )
        {   
        outPd->CopyData(inPd,this->PointIds.GetId(i),pts[0]);
        }
      verts->InsertNextCell(1,pts);
      }
    }
}

//
// Intersect with sub-vertices
//
int vtkPolyVertex::IntersectWithLine(float p1[3], float p2[3], 
                                    float tol, float& t, float x[3], 
                                    float pcoords[3], int& subId)
{
  static vtkVertex vertex;
  int subTest;

  for (subId=0; subId<this->Points.GetNumberOfPoints(); subId++)
    {
    vertex.Points.SetPoint(0,this->Points.GetPoint(subId));

    if ( vertex.IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest) )
      return 1;
    }

  return 0;
}

int vtkPolyVertex::Triangulate(int vtkNotUsed(index), vtkIdList &ptIds, 
                               vtkFloatPoints &pts)
{
  int subId;

  pts.Reset();
  ptIds.Reset();
  for (subId=0; subId<this->Points.GetNumberOfPoints(); subId++)
    {
    pts.InsertPoint(subId,this->Points.GetPoint(subId));
    ptIds.InsertId(subId,this->PointIds.GetId(subId));
    }
  return 1;
}

void vtkPolyVertex::Derivatives(int vtkNotUsed(subId), 
				float vtkNotUsed(pcoords)[3], 
				float *vtkNotUsed(values), 
				int dim, float *derivs)
{
  int i, idx;

  for (i=0; i<dim; i++)
    {
    idx = i*dim;
    derivs[idx] = 0.0;
    derivs[idx+1] = 0.0;
    derivs[idx+2] = 0.0;
    }
}

void vtkPolyVertex::Clip(float value, vtkFloatScalars *cellScalars, 
                         vtkPointLocator *locator, vtkCellArray *verts,
                         vtkPointData *inPD, vtkPointData *outPD,
                         int insideOut)
{
  float s, x[3];
  int pts[1], i;

  for ( i=0; i < this->Points.GetNumberOfPoints(); i++ )
    {
    s = cellScalars->GetScalar(i);

    if ( (!insideOut && s > value) || (insideOut && s <= value) )
      {
      this->Points.GetPoint(i,x);
      if ( (pts[0] = locator->IsInsertedPoint(x)) < 0 )
        {
        pts[0] = locator->InsertNextPoint(x);
        outPD->CopyData(inPD,this->PointIds.GetId(i),pts[0]);
        }
      verts->InsertNextCell(1,pts);
      }
    }
}
