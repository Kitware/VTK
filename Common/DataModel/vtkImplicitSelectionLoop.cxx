/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitSelectionLoop.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitSelectionLoop.h"

#include "vtkDoubleArray.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"

vtkStandardNewMacro(vtkImplicitSelectionLoop);
vtkCxxSetObjectMacro(vtkImplicitSelectionLoop, Loop,vtkPoints);

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
vtkImplicitSelectionLoop::~vtkImplicitSelectionLoop()
{
  if (this->Loop)
  {
    this->Loop->Delete();
  }
  this->Polygon->Delete();
  this->Polygon = NULL;
}

//----------------------------------------------------------------------------
#define VTK_DELTA 0.0001
// Generate plane equations only once to avoid a lot of extra work
void vtkImplicitSelectionLoop::Initialize()
{
  int i, numPts;
  double x[3], xProj[3];

  numPts = this->Loop->GetNumberOfPoints();
  this->Polygon->Points->SetDataTypeToDouble();
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

//----------------------------------------------------------------------------
// Evaluate plane equations. Return smallest absolute value.
double vtkImplicitSelectionLoop::EvaluateFunction(double x[3])
{
  int i, numPts;
  double xProj[3];
  double t, dist2, minDist2, closest[3];
  int inside=0;

  if ( this->InitializationTime < this->GetMTime() )
  {
    this->Initialize();
  }
  // Initialize may change the number of points
  numPts = this->Polygon->Points->GetNumberOfPoints();

  // project point onto plane
  vtkPlane::ProjectPoint(x, this->Origin, this->Normal, xProj);

  // determine whether it's in the selection loop and then evaluate point
  // in polygon only if absolutely necessary.
  if ( xProj[0] >= this->Bounds[0] && xProj[0] <= this->Bounds[1] &&
       xProj[1] >= this->Bounds[2] && xProj[1] <= this->Bounds[3] &&
       xProj[2] >= this->Bounds[4] && xProj[2] <= this->Bounds[5] &&
       this->Polygon->
       PointInPolygon(xProj , numPts,
                      vtkArrayDownCast<vtkDoubleArray>(this->Polygon->Points->GetData())->GetPointer(0),
                      this->Bounds,this->Normal) == 1 )
  {
    inside = 1;
  }

  // determine distance to boundary
  for (minDist2=VTK_DOUBLE_MAX,i=0; i<numPts; i++)
  {
    double p1[3], p2[3];
    this->Polygon->Points->GetPoint(i, p1);
    this->Polygon->Points->GetPoint((i+1)%numPts, p2);
    dist2 = vtkLine::DistanceToLine(xProj, p1, p2, t, closest);
    if ( dist2 < minDist2 )
    {
      minDist2 = dist2;
    }
  }

  minDist2 = static_cast<double>(sqrt(minDist2));
  return (inside ? -minDist2 : minDist2);
}

//----------------------------------------------------------------------------
// Evaluate gradient of the implicit function. Use a numerical scheme: evaluate
// the function at four points (O,O+dx,O+dy,O+dz) and approximate the gradient.
// It's damn slow.
void vtkImplicitSelectionLoop::EvaluateGradient(double x[3], double n[3])
{
  double xp[3], yp[3], zp[3], g0, gx, gy, gz;
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

//----------------------------------------------------------------------------
vtkMTimeType vtkImplicitSelectionLoop::GetMTime()
{
  vtkMTimeType mTime=this->vtkImplicitFunction::GetMTime();
  vtkMTimeType time;

  if ( this->Loop != NULL )
  {
    time = this->Loop->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkImplicitSelectionLoop::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Loop )
  {
    os << indent << "Loop of " << this->Loop->GetNumberOfPoints()
       << " points defined\n";
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
