/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitSelectionLoop.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImplicitSelectionLoop.h"
#include "vtkMath.h"
#include "vtkPlane.h"

// Instantiate object with no initial loop.
vtkImplicitSelectionLoop::vtkImplicitSelectionLoop()
{
  this->Loop = NULL;
  this->AutomaticNormalGeneration = 1;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
}

vtkImplicitSelectionLoop::~vtkImplicitSelectionLoop()
{
  if (this->Loop) this->Loop->Delete();
}

#define VTK_DELTA 0.0001
// Generate plane equations only once to avoid a lot of extra work
void vtkImplicitSelectionLoop::Initialize()
{
  int i, j, numPts;
  float x[3], x0[3], xProj[3];

  numPts = this->Loop->GetNumberOfPoints();
  this->Polygon.Points.SetNumberOfPoints(numPts);

  if ( this->AutomaticNormalGeneration )
    {
    // Make sure points define a loop with a normal
    vtkPolygon::ComputeNormal(this->Loop, this->Normal);
    if ( this->Normal[0] == 0.0 && this->Normal[1] == 0.0 && 
	 this->Normal[2] == 0.0 )
      {
      vtkErrorMacro(<<"Cannot determine inside/outside of loop");
      }
    }

  // Determine origin point by taking average
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  for (i=0; i<numPts; i++)
    {
    this->Loop->GetPoint(i, x);
    this->Origin[0] += x[0];
    this->Origin[1] += x[1];
    this->Origin[2] += x[2];
    }
  this->Origin[0] /= numPts; this->Origin[1] /= numPts; this->Origin[2] /= numPts;

  // Project points onto plane generating new coordinates
  for (i=0; i<numPts; i++)
    {
    this->Loop->GetPoint(i, x);
    vtkPlane::ProjectPoint(x, this->Origin, this->Normal, xProj);
    this->Polygon.Points.SetPoint(i, xProj);
    }

  this->Polygon.GetBounds(this->Bounds);
  this->DeltaX = VTK_DELTA*(this->Bounds[1]-this->Bounds[0]);
  this->DeltaY = VTK_DELTA*(this->Bounds[3]-this->Bounds[2]);
  this->DeltaZ = VTK_DELTA*(this->Bounds[5]-this->Bounds[4]);
  this->InitializationTime.Modified();
}
#undef VTK_DELTA

// Evaluate plane equations. Return smallest absolute value.
float vtkImplicitSelectionLoop::EvaluateFunction(float x[3])
{
  int i, numPts=this->Polygon.Points.GetNumberOfPoints();
  float ray[3], val, pcoords[2], xProj[3];
  float t, dist2, minDist2, closest[3];
  int inside=0;

  if ( this->InitializationTime < this->GetMTime() )
    {
    this->Initialize();
    }

  // project point onto plane
  vtkPlane::ProjectPoint(x, this->Origin, this->Normal, xProj);

  // determine whether it's in the selection loop and then evaluate point
  // in polygon only if absolutely necessary.
  if ( xProj[0] >= this->Bounds[0] && xProj[0] <= this->Bounds[1] &&
       xProj[1] >= this->Bounds[2] && xProj[1] <= this->Bounds[3] &&
       xProj[2] >= this->Bounds[4] && xProj[2] <= this->Bounds[5] &&
       this->Polygon.PointInPolygon(xProj, numPts,
                ((vtkFloatArray *)this->Polygon.Points.GetData())->GetPointer(0), 
                this->Bounds,this->Normal) == 1 )
    {
    inside = 1;
    }

  // determine distance to boundary
  for (minDist2=VTK_LARGE_FLOAT,i=0; i<numPts; i++)
    {
    dist2 = vtkLine::DistanceToLine(xProj,this->Polygon.Points.GetPoint(i),
			this->Polygon.Points.GetPoint((i+1)%numPts),t,closest);
    if ( dist2 < minDist2 ) minDist2 = dist2;
    }

  minDist2 = (float)sqrt(minDist2);
  return (inside ? -minDist2 : minDist2);
}

// Evaluate gradient of the implicit function. Use a numerical scheme: evaluate 
// the function at four points (O,O+dx,O+dy,O+dz) and approximate the gradient.
// It's damn slow.
void vtkImplicitSelectionLoop::EvaluateGradient(float x[3], float n[3])
{
  float xp[3], yp[3], zp[3], g0, gx, gy, gz;
  int i;

  g0 = this->EvaluateFunction(x); //side-effect is to compute DeltaX, Y, and Z

  for (i=0; i<3; i++)
    {
    xp[i] = yp[i] = zp[i] = x[i];
    }
  xp[0] += this->DeltaX;
  yp[1] += this->DeltaY;
  zp[2] += this->DeltaZ;

  gx = this->EvaluateFunction(xp);
  gy = this->EvaluateFunction(yp);
  gz = this->EvaluateFunction(zp);

  n[0] = (gx - g0) / this->DeltaX;
  n[1] = (gy - g0) / this->DeltaY;
  n[2] = (gz - g0) / this->DeltaZ;
}

unsigned long int vtkImplicitSelectionLoop::GetMTime()
{
  unsigned long mTime=this->vtkImplicitFunction::GetMTime();
  unsigned long time;

  if ( this->Loop != NULL )
    {
    time = this->Loop->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

void vtkImplicitSelectionLoop::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  if ( this->Loop )
    {
    os << indent << "Loop of " << this->Loop->GetNumberOfPoints()
       << "points defined\n";
    }
  else
    {
    os << indent << "Loop not defined\n";
    }

  os << indent << "Automatic Normal Generation: " 
     << (this->AutomaticNormalGeneration ? "On\n" : "Off\n");

  os << indent << "Normal: (" << this->Normal[0] << ", " 
     << this->Normal[1] << ", " << this->Normal[2] << ")\n";
}
