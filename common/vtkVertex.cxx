/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertex.cxx
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
#include "vtkVertex.h"
#include "vtkMath.h"
#include "vtkCellArray.h"
#include "vtkPointLocator.h"

// Description:
// Deep copy of cell.
vtkVertex::vtkVertex(const vtkVertex& p)
{
  this->Points = p.Points;
  this->PointIds = p.PointIds;
}

int vtkVertex::EvaluatePosition(float x[3], float closestPoint[3],
                              int& subId, float pcoords[3], 
                              float& dist2, float *weights)
{
  float *X;

  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  X = this->Points.GetPoint(0);
  closestPoint[0] = X[0]; closestPoint[1] = X[1]; closestPoint[2] = X[2];

  dist2 = vtkMath::Distance2BetweenPoints(X,x);
  weights[0] = 1.0;

  if (dist2 == 0.0)
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

void vtkVertex::EvaluateLocation(int& vtkNotUsed(subId), 
				 float vtkNotUsed(pcoords)[3], float x[3],
                                 float *weights)
{
  float *X = this->Points.GetPoint(0);
  x[0] = X[0];
  x[1] = X[1];
  x[2] = X[2];

  weights[0] = 1.0;
}

int vtkVertex::CellBoundary(int vtkNotUsed(subId), float pcoords[3], 
			    vtkIdList& pts)
{

  pts.Reset();
  pts.SetId(0,this->PointIds.GetId(0));

  if ( pcoords[0] != 0.0 )  
    return 0;
  else
    return 1;

}

void vtkVertex::Contour(float value, vtkFloatScalars *cellScalars, 
			vtkPointLocator *locator,
			vtkCellArray *verts, 
			vtkCellArray *vtkNotUsed(lines), 
			vtkCellArray *vtkNotUsed(polys), 
			vtkFloatScalars *scalars)
{
  if ( value == cellScalars->GetScalar(0) )
    {
    int pts[1];
    pts[0] = locator->InsertNextPoint(this->Points.GetPoint(0));
    scalars->InsertScalar(pts[0],value);
    verts->InsertNextCell(1,pts);
    }
}

// Project point on line. If it lies between 0<=t<=1 and distance off line
// is less than tolerance, intersection detected.
int vtkVertex::IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                                float x[3], float pcoords[3], int& subId)
{
  int i;
  float *X, ray[3], rayFactor, projXYZ[3];

  subId = 0;
  pcoords[1] = pcoords[2] = 0.0;

  X = this->Points.GetPoint(0);

  for (i=0; i<3; i++) ray[i] = p2[i] - p1[i];
  if (( rayFactor = vtkMath::Dot(ray,ray)) == 0.0 ) return 0;
//
//  Project each point onto ray. Determine whether point is within tolerance.
//
  t = (ray[0]*(X[0]-p1[0]) + ray[1]*(X[1]-p1[1]) + ray[2]*(X[2]-p1[2]))
      / rayFactor;

  if ( t >= 0.0 && t <= 1.0 )
    {
    for (i=0; i<3; i++) 
      {
      projXYZ[i] = p1[i] + t*ray[i];
      if ( fabs(X[i]-projXYZ[i]) > tol ) break;
      }

    if ( i > 2 ) // within tolerance 
      {
      pcoords[0] = 0.0;
      x[0] = X[0]; x[1] = X[1]; x[2] = X[2]; 
      return 1;
      }
    }

  pcoords[0] = -10.0;
  return 0;
}

int vtkVertex::Triangulate(int vtkNotUsed(index), vtkFloatPoints &pts)
{
  pts.Reset();
  pts.InsertPoint(0,this->Points.GetPoint(0));

  return 1;
}

void vtkVertex::Derivatives(int vtkNotUsed(subId), 
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

