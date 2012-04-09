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

vtkStandardNewMacro(vtkParametricSpline);

//----------------------------------------------------------------------------
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
  this->ParameterizeByLength = 1;
  
  this->InitializeTime = 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkParametricSpline::SetNumberOfPoints(vtkIdType numPts)
{
  if (!this->Points)
    {
    vtkPoints* pts = vtkPoints::New(VTK_DOUBLE);
    this->SetPoints(pts);
    pts->Delete();
    }
  this->Points->SetNumberOfPoints(numPts);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkParametricSpline::SetPoint(
  vtkIdType index, double x, double y, double z)
{
  if (this->Points)
    {
    this->Points->SetPoint(index, x, y, z);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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


//----------------------------------------------------------------------------
void vtkParametricSpline::Evaluate(double U[3], double Pt[3], double*)
{
  // make sure everything has been set up
  if ( this->InitializeTime < this->GetMTime () )
    {
    if ( ! this->Initialize() )
      {
      return;
      }
    }

  double t = (U[0] < 0.0 ? 0.0 : (U[0] > 1.0 ? 1.0 : U[0]));
  if ( this->Closed )
    {
    t *= this->ClosedLength;
    }
  else
    {
    t *= this->Length;
    }

  if (this->Length == 0 && this->Points && this->Points->GetNumberOfPoints() > 0)
    {
    this->Points->GetPoint(0, Pt);
    return;
    }

  // Evaluate the spline at the parameter t
  Pt[0] = this->XSpline->Evaluate(t);
  Pt[1] = this->YSpline->Evaluate(t);
  Pt[2] = this->ZSpline->Evaluate(t);
}

//----------------------------------------------------------------------------
double vtkParametricSpline::EvaluateScalar(double u[3], double*, double *)
{
  // make sure everything has been set up
  if ( this->InitializeTime < this->GetMTime () )
    {
    if ( ! this->Initialize() )
      {
      return 0.0;
      }
    }

  return u[0]; //simply parametric value
}

//----------------------------------------------------------------------------
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
  double xPrev[3], x[3], len;
  vtkIdType npts = this->Points->GetNumberOfPoints();

  if ( npts < 1)
    {
    vtkErrorMacro("Please specify at least one point.");
    return 0;
    }
  if ( npts < 2 )
    {
    this->Length = 0;
    this->ClosedLength = 0;
    // If number of points == 1, then we simply generate a single point.
    return 1;
    }

  if ( this->ParameterizeByLength )
    {
    this->Points->GetPoint(0,xPrev);
    this->Length = 0.0;
    for ( i = 1; i < npts; ++i )
      {
      this->Points->GetPoint(i,x);
      len = sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
      // No need to complain if there are coincident points, we indeed handle
      // them correctly.
      //if ( len <= 0.0 )
      //  {
      //  vtkErrorMacro("Spline must have non-coincident points");
      //  return 0; //failure
      //  }
      this->Length += len;
      xPrev[0]=x[0]; xPrev[1]=x[1]; xPrev[2]=x[2];
      }
    if ( this->Length <= 0.0 )
      {
      // Zero length spline is same a single point.
      this->ClosedLength = 0.0;
      return 1;
      }
    if ( this->Closed )
      {
      this->Points->GetPoint(0,x);
      this->ClosedLength = this->Length +
        sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
      }
    }
  else
    {
    this->Length = npts - 1;
    if ( this->Closed )
      {
      this->ClosedLength = npts;
      }
    }

  this->XSpline->RemoveAllPoints();
  this->YSpline->RemoveAllPoints();
  this->ZSpline->RemoveAllPoints();

    // Specify the parametric range that the spline can take
  if ( ! this->Closed )
    {
    this->XSpline->SetParametricRange(0.0,this->Length);
    this->YSpline->SetParametricRange(0.0,this->Length);
    this->ZSpline->SetParametricRange(0.0,this->Length);
    }
  else
    {
    this->XSpline->SetParametricRange(0.0,this->ClosedLength);
    this->YSpline->SetParametricRange(0.0,this->ClosedLength);
    this->ZSpline->SetParametricRange(0.0,this->ClosedLength);
    }

  // Now we insert points into the splines with the parametric coordinate
  // based on (polyline) length. We keep track of the parametric coordinates
  // of the points for later point interpolation.
  if ( this->ParameterizeByLength )
    {
    this->Points->GetPoint(0,xPrev);
    for ( len = 0.0, i = 0; i < npts; ++i )
      {
      this->Points->GetPoint(i,x);
      len += sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));

      this->XSpline->AddPoint(len,x[0]);
      this->YSpline->AddPoint(len,x[1]);
      this->ZSpline->AddPoint(len,x[2]);

      xPrev[0]=x[0]; xPrev[1]=x[1]; xPrev[2]=x[2];
      }
    }
  else
    {
    for ( i = 0; i < npts; ++i )
      {
      this->Points->GetPoint(i,x);
      this->XSpline->AddPoint(i,x[0]);
      this->YSpline->AddPoint(i,x[1]);
      this->ZSpline->AddPoint(i,x[2]);
      }
    }

  this->InitializeTime = this->GetMTime();
  return 1;
}


//----------------------------------------------------------------------------
void vtkParametricSpline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Points: ";
  if ( this->Points )
    {
    os << this->Points << "\n";
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "X Spline: ";
  if ( this->XSpline )
    {
    os << this->XSpline << "\n";
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "Y Spline: ";
  if ( this->YSpline )
    {
    os << this->YSpline << "\n";
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "Z Spline: ";
  if ( this->ZSpline )
    {
    os << this->ZSpline << "\n";
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Closed: " << (this->Closed ? "On\n" : "Off\n");
  os << indent << "Left Constraint: " << this->LeftConstraint << "\n";
  os << indent << "Right Constraint: " << this->RightConstraint << "\n";
  os << indent << "Left Value: " << this->LeftValue << "\n";
  os << indent << "Right Value: " << this->RightValue << "\n";
  os << indent << "Parameterize by length: "
     << (this->ParameterizeByLength ? "On\n" : "Off\n");
}
