/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitSelectionLoop.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkImplicitSelectionLoop.h"
#include "vtkMath.h"
#include "vtkPlane.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImplicitSelectionLoop* vtkImplicitSelectionLoop::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImplicitSelectionLoop");
  if(ret)
    {
    return (vtkImplicitSelectionLoop*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImplicitSelectionLoop;
}




// Instantiate object with no initial loop.
vtkImplicitSelectionLoop::vtkImplicitSelectionLoop()
{
  this->Loop = NULL;
  this->AutomaticNormalGeneration = 1;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->Polygon = vtkPolygon::New();
}

vtkImplicitSelectionLoop::~vtkImplicitSelectionLoop()
{
  if (this->Loop)
    {
    this->Loop->Delete();
    }
  this->Polygon->Delete();
  this->Polygon = NULL;
}

#define VTK_DELTA 0.0001
// Generate plane equations only once to avoid a lot of extra work
void vtkImplicitSelectionLoop::Initialize()
{
  int i, numPts;
  float x[3], xProj[3];

  numPts = this->Loop->GetNumberOfPoints();
  this->Polygon->Points->SetNumberOfPoints(numPts);

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
    this->Polygon->Points->SetPoint(i, xProj);
    }

  this->Polygon->GetBounds(this->Bounds);
  this->DeltaX = VTK_DELTA*(this->Bounds[1]-this->Bounds[0]);
  this->DeltaY = VTK_DELTA*(this->Bounds[3]-this->Bounds[2]);
  this->DeltaZ = VTK_DELTA*(this->Bounds[5]-this->Bounds[4]);
  this->InitializationTime.Modified();
}
#undef VTK_DELTA

// Evaluate plane equations. Return smallest absolute value.
float vtkImplicitSelectionLoop::EvaluateFunction(float x[3])
{
  int i, numPts=this->Polygon->Points->GetNumberOfPoints();
  float xProj[3];
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
       this->Polygon->PointInPolygon(xProj, numPts,
                ((vtkFloatArray *)this->Polygon->Points->GetData())->GetPointer(0), 
                this->Bounds,this->Normal) == 1 )
    {
    inside = 1;
    }

  // determine distance to boundary
  for (minDist2=VTK_LARGE_FLOAT,i=0; i<numPts; i++)
    {
    dist2 = vtkLine::DistanceToLine(xProj,this->Polygon->Points->GetPoint(i),
			this->Polygon->Points->GetPoint((i+1)%numPts),t,closest);
    if ( dist2 < minDist2 )
      {
      minDist2 = dist2;
      }
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
