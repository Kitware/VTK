/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParametricSpline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParametricSpline.h"
#include "vtkObjectFactory.h"
#include "vtkCardinalSpline.h"
#include "vtkPoints.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkParametricSpline, "1.1");
vtkStandardNewMacro(vtkParametricSpline);

vtkParametricSpline::vtkParametricSpline()
{
  this->MinimumU = 0;
  this->MaximumU = 1.0;
  this->JoinU = 0;

  this->Points = NULL;

  this->XSpline = vtkCardinalSpline::New();
  this->YSpline = vtkCardinalSpline::New();
  this->ZSpline = vtkCardinalSpline::New();
  
  this->Closed = 0;
  this->LeftConstraint = 1;
  this->LeftValue = 0.0;
  this->RightConstraint = 1;
  this->RightValue = 0.0;
  
  this->InitializeTime = 0;
}

vtkParametricSpline::~vtkParametricSpline()
{
  if (this->Points)
            {
    this->Points->Delete();
    }
  if (this->XSpline)
    {
    this->XSpline->Delete();
    }
  if (this->YSpline)
    {
    this->YSpline->Delete();
    }
  if (this->ZSpline)
    {
    this->ZSpline->Delete();
    }
}

void vtkParametricSpline::SetPoints(vtkPoints *pts)
{
  if ( pts != this->Points )
    {
    if ( this->Points != NULL )
      {
      this->Points->Delete();
      }
    this->Points = pts;
    if ( this->Points != NULL )
      {
      this->Points->Register(this);
      }
    this->Modified();
    }
}

void vtkParametricSpline::SetXSpline(vtkSpline *s)
{
  if ( s != this->XSpline )
    {
    if ( this->XSpline != NULL )
      {
      this->XSpline->Delete();
      }
    this->XSpline = s;
    if ( this->XSpline != NULL )
      {
      this->XSpline->Register(this);
      }
    this->Modified();
    }
}

void vtkParametricSpline::SetYSpline(vtkSpline *s)
{
  if ( s != this->YSpline )
    {
    if ( this->YSpline != NULL )
      {
      this->YSpline->Delete();
      }
    this->YSpline = s;
    if ( this->YSpline != NULL )
      {
      this->YSpline->Register(this);
      }
    this->Modified();
    }
}

void vtkParametricSpline::SetZSpline(vtkSpline *s)
{
  if ( s != this->ZSpline )
    {
    if ( this->ZSpline != NULL )
      {
      this->ZSpline->Delete();
      }
    this->ZSpline = s;
    if ( this->ZSpline != NULL )
      {
      this->ZSpline->Register(this);
      }
    this->Modified();
    }
}


void vtkParametricSpline::Evaluate(double U[3], double Pt[3], double*)
{
  // make sure everything has been set up
  if (this->InitializeTime < this->GetMTime ())
    {
    if ( ! this->Initialize() )
      {
      return;
      }
    }

  double t = (U[0] < 0.0 ? 0.0 : (U[0] > 1.0 ? 1.0 : U[0]));

  // Evaluate the spline at the parameter t
  Pt[0] = this->XSpline->Evaluate(t);
  Pt[1] = this->YSpline->Evaluate(t);
  Pt[2] = this->ZSpline->Evaluate(t);
}

double vtkParametricSpline::EvaluateScalar(double u[3], double*, double *)
{
  // make sure everything has been set up
  if (this->InitializeTime < this->GetMTime ())
    {
    if ( ! this->Initialize() )
      {
      return 0.0;
      }
    }

  return u[0]; //simply parametric value
}

// Configure the splines for evaluation
int vtkParametricSpline::Initialize()
{
  // Check to make sure splines are available
  if ( !this->XSpline || !this->YSpline || !this->ZSpline )
    {
    vtkErrorMacro("Please specify splines");
    return 0;
    }
  if ( !this->Points )
    {
    vtkErrorMacro("Please specify points");
    return 0;
    }
  
  // Make sure that the splines are consistent with this instance
  this->XSpline->SetClosed(this->GetClosed());
  this->XSpline->SetLeftConstraint(this->GetLeftConstraint());
  this->XSpline->SetRightConstraint(this->GetRightConstraint());
  this->XSpline->SetLeftValue(this->GetLeftValue());
  this->XSpline->SetRightValue(this->GetRightValue());
  
  this->YSpline->SetClosed(this->GetClosed());
  this->YSpline->SetLeftConstraint(this->GetLeftConstraint());
  this->YSpline->SetRightConstraint(this->GetRightConstraint());
  this->YSpline->SetLeftValue(this->GetLeftValue());
  this->YSpline->SetRightValue(this->GetRightValue());
  
  this->ZSpline->SetClosed(this->GetClosed());
  this->ZSpline->SetLeftConstraint(this->GetLeftConstraint());
  this->ZSpline->SetRightConstraint(this->GetRightConstraint());
  this->ZSpline->SetLeftValue(this->GetLeftValue());
  this->ZSpline->SetRightValue(this->GetRightValue());
  

  // Construct the splines, parameterized by length
  vtkIdType i;
  double xPrev[3], x[3], length=0.0, len, t;
  this->Points->GetPoint(0,xPrev);
  vtkIdType npts = this->Points->GetNumberOfPoints();
  vtkIdType totalPts = npts;
  vtkIdType idx;
  
  if ( this->Closed )
    {
    totalPts = npts + 1; //effect is to adjust parametric space
    }

  for (i=1; i < totalPts; i++)
    {
    idx = i % npts;
    this->Points->GetPoint(idx,x);
    len = sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
    if ( len <= 0.0 )
      {
      vtkErrorMacro("Spline must have non-coincident points");
      return 0; //failure
      }
    length += len;
    xPrev[0]=x[0]; xPrev[1]=x[1]; xPrev[2]=x[2];
    }
  if ( length <= 0.0 )
    {
    vtkErrorMacro("Spline must have non-zero length");
    return 0; //failure
    }

  // Now we insert points into the splines with the parametric coordinate
  // based on (polyline) length. We keep track of the parametric coordinates
  // of the points for later point interpolation.
  this->Points->GetPoint(0,xPrev);
  for (len=0,i=0; i < totalPts; i++)
    {
    idx = i % npts;
    this->Points->GetPoint(idx,x);
    len += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
    t = len/length;

    this->XSpline->AddPoint(t,x[0]);
    this->YSpline->AddPoint(t,x[1]);
    this->ZSpline->AddPoint(t,x[2]);
    
    xPrev[0]=x[0]; xPrev[1]=x[1]; xPrev[2]=x[2];
    }

  this->InitializeTime = this->GetMTime();
  return 1;
}


void vtkParametricSpline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
