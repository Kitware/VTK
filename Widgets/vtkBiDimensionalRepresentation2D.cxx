/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiDimensionalRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBiDimensionalRepresentation2D.h"
#include "vtkHandleRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkRenderer.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkTextProperty.h"
#include "vtkWindow.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkProperty2D.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkObjectFactory.h"
#include "vtkInteractorObserver.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkBiDimensionalRepresentation2D);


//----------------------------------------------------------------------
vtkBiDimensionalRepresentation2D::vtkBiDimensionalRepresentation2D() : vtkBiDimensionalRepresentation()
{
  // Create the geometry for the two axes
  this->LineCells = vtkCellArray::New();
  this->LineCells->InsertNextCell(2);
  this->LineCells->InsertCellPoint(0);
  this->LineCells->InsertCellPoint(1);
  this->LineCells->InsertNextCell(2);
  this->LineCells->InsertCellPoint(2);
  this->LineCells->InsertCellPoint(3);
  this->LinePoints = vtkPoints::New();
  this->LinePoints->SetNumberOfPoints(4);
  this->LinePolyData = vtkPolyData::New();
  this->LinePolyData->SetPoints(this->LinePoints);
  this->LinePolyData->SetLines(this->LineCells);
  this->LineMapper = vtkPolyDataMapper2D::New();
  this->LineMapper->SetInput(this->LinePolyData);
  this->LineProperty = vtkProperty2D::New();
  this->LineActor = vtkActor2D::New();
  this->LineActor->SetProperty(this->LineProperty);
  this->LineActor->SetMapper(this->LineMapper);
  this->SelectedLineProperty = vtkProperty2D::New();
  this->SelectedLineProperty->SetColor(0.0,1.0,0.0);
  this->SelectedLineProperty->SetLineWidth(2.0);

  this->TextProperty = vtkTextProperty::New();
  this->TextProperty->SetBold(1);
  this->TextProperty->SetItalic(1);
  this->TextProperty->SetShadow(1);
  this->TextProperty->SetFontFamilyToArial();
  this->TextMapper = vtkTextMapper::New();
  this->TextMapper->SetTextProperty(this->TextProperty);
  this->TextMapper->SetInput("0.0");
  this->TextActor = vtkActor2D::New();
  this->TextActor->SetMapper(this->TextMapper);
  this->TextActor->VisibilityOff();
}

//----------------------------------------------------------------------
vtkBiDimensionalRepresentation2D::~vtkBiDimensionalRepresentation2D()
{
  this->LineCells->Delete();
  this->LinePoints->Delete();
  this->LinePolyData->Delete();
  this->LineMapper->Delete();
  this->LineProperty->Delete();
  this->LineActor->Delete();
  this->SelectedLineProperty->Delete();
  this->TextProperty->Delete();
  this->TextMapper->Delete();
  this->TextActor->Delete();
}


//----------------------------------------------------------------------
int vtkBiDimensionalRepresentation2D::
ComputeInteractionState(int X, int Y, int modify)
{
  this->Modifier = modify;

  // Check if we are on end points. The handles must tell us to ensure
  // consistent state.
  int p1State = this->Point1Representation->ComputeInteractionState(X,Y,0);
  int p2State = this->Point2Representation->ComputeInteractionState(X,Y,0);
  int p3State = this->Point3Representation->ComputeInteractionState(X,Y,0);
  int p4State = this->Point4Representation->ComputeInteractionState(X,Y,0);
  if ( p1State == vtkHandleRepresentation::Nearby )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::NearP1;
    }
  else if ( p2State == vtkHandleRepresentation::Nearby )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::NearP2;
    }
  else if ( p3State == vtkHandleRepresentation::Nearby )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::NearP3;
    }
  else if ( p4State == vtkHandleRepresentation::Nearby )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::NearP4;
    }
  else
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::Outside;
    }

  // Okay if we're near a handle return, otherwise test edges.
  if ( this->InteractionState != vtkBiDimensionalRepresentation2D::Outside )
    {
    return this->InteractionState;
    }

  // See if we are near the edges. Requires separate computation.
  double pos1[3], pos2[3], pos3[3], pos4[3];
  this->GetPoint1DisplayPosition(pos1);
  this->GetPoint2DisplayPosition(pos2);
  this->GetPoint3DisplayPosition(pos3);
  this->GetPoint4DisplayPosition(pos4);

  double p1[3], p2[3], p3[3], p4[3], xyz[3];
  double t, closest[3];
  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  p1[0] = static_cast<double>(pos1[0]);
  p1[1] = static_cast<double>(pos1[1]);
  p2[0] = static_cast<double>(pos2[0]);
  p2[1] = static_cast<double>(pos2[1]);
  p3[0] = static_cast<double>(pos3[0]);
  p3[1] = static_cast<double>(pos3[1]);
  p4[0] = static_cast<double>(pos4[0]);
  p4[1] = static_cast<double>(pos4[1]);
  xyz[2] = p1[2] = p2[2] = p3[2] = p4[2] = 0.0;

  double tol2 = this->Tolerance*this->Tolerance;

  // Compute intersection point.
  double uIntersect, vIntersect;
  vtkLine::Intersection(p1, p2, p3, p4, uIntersect, vIntersect);

  // Check if we are on edges
  int onL1 = (vtkLine::DistanceToLine(xyz,p1,p2,t,closest) <= tol2);
  int onL2 = (vtkLine::DistanceToLine(xyz,p3,p4,t,closest) <= tol2);

  double xyzParam;

  if ( onL1 && onL2 )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::OnCenter;
    }
  else if ( onL1 )
    {
    if (p1[0] != p2[0])
      {
      xyzParam = (xyz[0] - p1[0]) / (p2[0] - p1[0]);
      if (xyzParam < uIntersect)
        {
        // closer to p1
        if (xyzParam < (uIntersect*0.5))
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL1Outer;
          }
        else
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL1Inner;
          }
        }
      else
        {
        // closer to p2
        if (xyzParam > ((1+uIntersect)*0.5))
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL1Outer;
          }
        else
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL1Inner;
          }
        }
      }
    else
      {
      xyzParam = (xyz[1] - p1[1]) / (p2[1] - p1[1]);
      if (xyzParam < uIntersect)
        {
        // closer to p1
        if (xyzParam < (uIntersect*0.5))
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL1Outer;
          }
        else
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL1Inner;
          }
        }
      else
        {
        // closer to p2
        if (xyzParam > ((1+uIntersect)*0.5))
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL1Outer;
          }
        else
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL1Inner;
          }
        }
      }
    }
  else if ( onL2 )
    {
    if (p3[0] != p4[0])
      {
      xyzParam = (xyz[0] - p3[0]) / (p4[0] - p3[0]);
      if (xyzParam < vIntersect)
        {
        // closer to p3
        if (xyzParam < (vIntersect*0.5))
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL2Outer;
          }
        else
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL2Inner;
          }
        }
      else
        {
        // closer to p4
        if (xyzParam > ((1+vIntersect)*0.5))
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL2Outer;
          }
        else
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL2Inner;
          }
        }
      }
    else
      {
      xyzParam = (xyz[1] - p3[1]) / (p4[1] - p3[1]);
      if (xyzParam < vIntersect)
        {
        // closer to p3
        if (xyzParam < (vIntersect*0.5))
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL2Outer;
          }
        else
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL2Inner;
          }
        }
      else
        {
        // closer to p4
        if (xyzParam > ((1+vIntersect)*0.5))
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL2Outer;
          }
        else
          {
          this->InteractionState = vtkBiDimensionalRepresentation2D::OnL2Inner;
          }
        }
      }
    }
  else
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::Outside;
    this->Modifier = 0;
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::StartWidgetDefinition(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;

  this->SetPoint1DisplayPosition(pos);
  this->SetPoint2DisplayPosition(pos);
  this->SetPoint3DisplayPosition(pos);
  this->SetPoint4DisplayPosition(pos);

  this->StartEventPosition[0] = pos[0];
  this->StartEventPosition[1] = pos[1];
  this->StartEventPosition[2] = pos[2];
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::Point2WidgetInteraction(double e[2])
{
  double pos[3],p1[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;

  // Make sure that the two points are not coincident
  this->GetPoint1DisplayPosition(p1);
  if ( ((pos[0]-p1[0])*(pos[0]-p1[0]) + (pos[1]-p1[1])*(pos[1]-p1[1])) < 2 )
    {
    pos[0] += 2;
    }
  this->SetPoint2DisplayPosition(pos);
}

//----------------------------------------------------------------------
// This method is called when Point3 is to be manipulated. Note that Point3
// and Point4 are constrained relative to Line1. As a result, manipulating P3
// results in manipulating P4.
void vtkBiDimensionalRepresentation2D::Point3WidgetInteraction(double e[2])
{
  double p1[3], p2[3], p3[3], p4[3];
  double slope1[3], slope2[3];

  // Start by getting the coordinates (P1,P2) defining Line1. Also get
  // characterisitics of Line1 including its slope, etc.
  this->GetPoint1WorldPosition(p1);
  this->GetPoint2WorldPosition(p2);
  slope1[0] = p2[0] - p1[0];
  slope1[1] = p2[1] - p1[1];
  slope2[0] = -slope1[1];
  slope2[1] =  slope1[0];
  slope2[2] = 0.0;
  vtkMath::Normalize(slope2);

  // The current position of P3 is constrained to lie along Line1. Also,
  // P4 is placed on the opposite side of Line1.
  double pw[4], t, closest[3];
  if ( this->Renderer )
    {
    this->Renderer->SetDisplayPoint(e[0],e[1],0.0);
    this->Renderer->DisplayToWorld();
    this->Renderer->GetWorldPoint(pw);
    }
  double dist = sqrt(vtkLine::DistanceToLine(pw,p1,p2,t,closest));

  // Set the positions of P3 and P4.
  p3[0] = closest[0] + dist*slope2[0];
  p3[1] = closest[1] + dist*slope2[1];
  p3[2] = pw[2];
  this->SetPoint3WorldPosition(p3);

  p4[0] = closest[0] - dist*slope2[0];
  p4[1] = closest[1] - dist*slope2[1];
  p4[2] = pw[2];
  this->SetPoint4WorldPosition(p4);
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::StartWidgetManipulation(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  if ( this->Renderer )
    {
    this->Renderer->SetDisplayPoint(e[0],e[1],0.0);
    this->Renderer->DisplayToWorld();
    this->Renderer->GetWorldPoint(this->StartEventPositionWorld);
    }

  this->GetPoint1WorldPosition(this->P1World);
  this->GetPoint2WorldPosition(this->P2World);
  this->GetPoint3WorldPosition(this->P3World);
  this->GetPoint4WorldPosition(this->P4World);

  int i;
  for (i=0; i<3; i++)
    {
    this->P21World[i] = this->P2World[i] - this->P1World[i];
    this->P43World[i] = this->P4World[i] - this->P3World[i];
    }

  vtkLine::Intersection(this->P1World,this->P2World,
                        this->P3World,this->P4World,
                        this->T21,this->T43);

  // Compute the center point
  for (i=0; i<3; i++)
    {
    this->CenterWorld[i] = ((this->P1World[i] + this->T21*this->P21World[i]) +
                            (this->P3World[i] + this->T43*this->P43World[i]))/2.0;
    }
}

//----------------------------------------------------------------------
// This handles all the nasty special cases when the length of the arms of the
// bidimensional widget become zero. Basically the method prevents the arms
// from getting too short.
void vtkBiDimensionalRepresentation2D::ProjectOrthogonalPoint(double x[4], double y[3], double x1[3], double x2[3],
                                                              double x21[3], double dir, double xP[3])
{
  double t, closest[3], slope[3], dist;

  // determine the distance from the other (orthogonal) line
  dist = dir * sqrt(vtkLine::DistanceToLine(x,x1,x2,t,closest));

  // get the closest point on the other line, use its "mate" point to define the projection point,
  // this keeps everything orthogonal.
  vtkLine::DistanceToLine(y,x1,x2,t,closest);

  // Project the point "dist" orthogonal to ray x21.
  // Define an orthogonal line.
  slope[0] = -x21[1];
  slope[1] =  x21[0];
  slope[2] = 0.0;

  // Project out the right distance along the calculated slope
  vtkMath::Normalize(slope);
  xP[0] = closest[0] + dist*slope[0];
  xP[1] = closest[1] + dist*slope[1];
  xP[2] = closest[2] + dist*slope[2];

  // Check to see what side the projection is on, clamp if necessary. Note that closest is modified so that the
  // arms don't end up with zero length.
  if ( ((xP[0]-closest[0])*(x[0]-closest[0]) + (xP[1]-closest[1])*(x[1]-closest[1]) + (xP[2]-closest[2])*(x[2]-closest[2])) < 0.0 )
    {
    // Convert closest point to display coordinates
    double c1[3], c2[3], c21[3], cNew[3], xPNew[4];
    this->Renderer->SetWorldPoint(closest[0],closest[1],closest[2],1.0);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(c1);
    // Convert vector in world space to display space
    this->Renderer->SetWorldPoint(closest[0]+dir*slope[0],closest[1]+dir*slope[1],closest[2]+dir*slope[2],1.0);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(c2);
    c21[0] = c2[0] - c1[0];
    c21[1] = c2[1] - c1[1];
    c21[2] = c2[2] - c1[2];
    vtkMath::Normalize(c21);

    // Perform vector addition in display space to get new point
    cNew[0] = c1[0] + c21[0];
    cNew[1] = c1[1] + c21[1];
    cNew[2] = c1[2] + c21[2];

    this->Renderer->SetDisplayPoint(cNew[0],cNew[1],cNew[2]);
    this->Renderer->DisplayToWorld();
    this->Renderer->GetWorldPoint(xPNew);

    xP[0] = xPNew[0];
    xP[1] = xPNew[1];
    xP[2] = xPNew[2];
    }
}

//----------------------------------------------------------------------
// This method is tricky because it is constrained by Line1 and Line2.
// This method is invoked after all four points have been placed.
void vtkBiDimensionalRepresentation2D::WidgetInteraction(double e[2])
{
  // Depending on the state, different motions are allowed.
  if ( this->InteractionState == Outside || ! this->Renderer )
    {
    return;
    }

  // Okay, go to work, convert this event to world coordinates
  double pw[4], t, closest[3];
  double p1[3], p2[3], p3[3], p4[3];
  this->Renderer->SetDisplayPoint(e[0],e[1],0.0);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(pw);

  // depending on the state, perform different operations
  if ( this->InteractionState == OnCenter )
    {
    for (int i=0; i<3; i++)
      {
      p1[i] = this->P1World[i] + (pw[i]-this->StartEventPositionWorld[i]);
      p2[i] = this->P2World[i] + (pw[i]-this->StartEventPositionWorld[i]);
      p3[i] = this->P3World[i] + (pw[i]-this->StartEventPositionWorld[i]);
      p4[i] = this->P4World[i] + (pw[i]-this->StartEventPositionWorld[i]);
      }
    this->SetPoint1WorldPosition(p1);
    this->SetPoint2WorldPosition(p2);
    this->SetPoint3WorldPosition(p3);
    this->SetPoint4WorldPosition(p4);
    }
  else if ( this->InteractionState == OnL1Outer ||
            this->InteractionState == OnL2Outer) //rotate the representation
    {
    // compute rotation angle and center of rotation
    double sc[3], ec[3], p1c[3], p2c[3], p3c[3], p4c[3];
    for (int i=0; i<3; i++)
      {
      sc[i] = this->StartEventPositionWorld[i] - this->CenterWorld[i];
      ec[i] = pw[i] - this->CenterWorld[i];
      p1c[i] = this->P1World[i] - this->CenterWorld[i];
      p2c[i] = this->P2World[i] - this->CenterWorld[i];
      p3c[i] = this->P3World[i] - this->CenterWorld[i];
      p4c[i] = this->P4World[i] - this->CenterWorld[i];
      }
    double theta = atan2(ec[1],ec[0]) - atan2(sc[1],sc[0]);
    double r1 = vtkMath::Norm(p1c);
    double r2 = vtkMath::Norm(p2c);
    double r3 = vtkMath::Norm(p3c);
    double r4 = vtkMath::Norm(p4c);
    double theta1 = atan2(p1c[1],p1c[0]);
    double theta2 = atan2(p2c[1],p2c[0]);
    double theta3 = atan2(p3c[1],p3c[0]);
    double theta4 = atan2(p4c[1],p4c[0]);

    //rotate the four points
    p1[0] = this->CenterWorld[0] + r1*cos(theta+theta1);
    p1[1] = this->CenterWorld[1] + r1*sin(theta+theta1);
    p2[0] = this->CenterWorld[0] + r2*cos(theta+theta2);
    p2[1] = this->CenterWorld[1] + r2*sin(theta+theta2);
    p3[0] = this->CenterWorld[0] + r3*cos(theta+theta3);
    p3[1] = this->CenterWorld[1] + r3*sin(theta+theta3);
    p4[0] = this->CenterWorld[0] + r4*cos(theta+theta4);
    p4[1] = this->CenterWorld[1] + r4*sin(theta+theta4);
    p1[2] = this->P1World[2];
    p2[2] = this->P2World[2];
    p3[2] = this->P3World[2];
    p4[2] = this->P4World[2];

    this->SetPoint1WorldPosition(p1);
    this->SetPoint2WorldPosition(p2);
    this->SetPoint3WorldPosition(p3);
    this->SetPoint4WorldPosition(p4);
    }
  else if ( this->InteractionState == OnL1Inner )
    {
    vtkLine::DistanceToLine(pw,this->P3World,this->P4World,t,closest);
    t = ( t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t) );
    for (int i=0; i<3; i++)
      {
      p1[i] = this->P1World[i] + (t-this->T43)*this->P43World[i];
      p2[i] = this->P2World[i] + (t-this->T43)*this->P43World[i];
      }

    // Set the positions of P1 and P2.
    this->SetPoint1WorldPosition(p1);
    this->SetPoint2WorldPosition(p2);
    }
  else if ( this->InteractionState == OnL2Inner )
    {
    vtkLine::DistanceToLine(pw,this->P1World,this->P2World,t,closest);
    t = ( t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t) );
    for (int i=0; i<3; i++)
      {
      p3[i] = this->P3World[i] + (t-this->T21)*this->P21World[i];
      p4[i] = this->P4World[i] + (t-this->T21)*this->P21World[i];
      }

    // Set the positions of P3 and P4.
    this->SetPoint3WorldPosition(p3);
    this->SetPoint4WorldPosition(p4);
    }
  else if ( this->InteractionState == NearP1 )
    {
    this->ProjectOrthogonalPoint(pw,this->P2World,this->P3World,this->P4World,this->P43World,-1,p1);
    this->SetPoint1WorldPosition(p1);
    }
  else if ( this->InteractionState == NearP2 )
    {
    this->ProjectOrthogonalPoint(pw,this->P1World,this->P3World,this->P4World,this->P43World,1,p2);
    this->SetPoint2WorldPosition(p2);
    }
  else if ( this->InteractionState == NearP3 )
     {
    this->ProjectOrthogonalPoint(pw,this->P4World,this->P1World,this->P2World,this->P21World,1,p3);
    this->SetPoint3WorldPosition(p3);
    }
  else if ( this->InteractionState == NearP4 )
    {
    this->ProjectOrthogonalPoint(pw,this->P3World,this->P1World,this->P2World,this->P21World,-1,p4);
    this->SetPoint4WorldPosition(p4);
    } //near P4
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime ||
       this->Point1Representation->GetMTime() > this->BuildTime ||
       this->Point2Representation->GetMTime() > this->BuildTime ||
       this->Point3Representation->GetMTime() > this->BuildTime ||
       this->Point4Representation->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    // Make sure that tolerance is consistent between handles and this representation
    this->Point1Representation->SetTolerance(this->Tolerance);
    this->Point2Representation->SetTolerance(this->Tolerance);
    this->Point3Representation->SetTolerance(this->Tolerance);
    this->Point4Representation->SetTolerance(this->Tolerance);

    // Now bring the lines up to date
    if ( ! this->Line1Visibility )
      {
      return;
      }

    char distStr1[256], distStr2[256];
    double p1[3], p2[3], p3[3], p4[3];
    this->GetPoint1DisplayPosition(p1);
    this->GetPoint2DisplayPosition(p2);
    this->GetPoint3DisplayPosition(p3);
    this->GetPoint4DisplayPosition(p4);

    double wp1[3], wp2[3], wp3[3], wp4[3];
    this->GetPoint1WorldPosition(wp1);
    this->GetPoint2WorldPosition(wp2);
    this->GetPoint3WorldPosition(wp3);
    this->GetPoint4WorldPosition(wp4);

    this->LinePoints->SetPoint(0,p1);
    this->LinePoints->SetPoint(1,p2);
    this->LinePoints->SetPoint(2,p3);
    this->LinePoints->SetPoint(3,p4);
    this->LinePoints->Modified();

    this->LineCells->Reset();
    this->LineCells->InsertNextCell(2);
    this->LineCells->InsertCellPoint(0);
    this->LineCells->InsertCellPoint(1);

    if ( this->Line2Visibility )
      {
      this->LineCells->InsertNextCell(2);
      this->LineCells->InsertCellPoint(2);
      this->LineCells->InsertCellPoint(3);
      }

    double line1Dist = sqrt(vtkMath::Distance2BetweenPoints(wp1, wp2));
    double line2Dist = 0;
    if (this->Line2Visibility)
      {
      line2Dist = sqrt(vtkMath::Distance2BetweenPoints(wp3, wp4));
      }
    vtksys_ios::ostringstream label;
    if (this->IDInitialized)
      {
      label << this->ID << ": ";
      }
    sprintf(distStr1,this->LabelFormat, line1Dist);
    sprintf(distStr2,this->LabelFormat, line2Dist);

    if (line1Dist > line2Dist)
      {
      label << distStr1 << " x " << distStr2;
      }
    else
      {
      label << distStr2 << " x " << distStr1;
      }
    this->TextMapper->SetInput(label.str().c_str());

    // Adjust the font size
    int stringSize[2], *winSize = this->Renderer->GetSize();
    vtkTextMapper::SetRelativeFontSize(this->TextMapper, this->Renderer, winSize,
                                       stringSize, 0.015);

    int maxX = VTK_INT_MIN, maxY = VTK_INT_MIN;
    if (p1[1] > maxY)
      {
      maxX = static_cast<int>(p1[0]);
      maxY = static_cast<int>(p1[1]);
      }
    if (p2[1] > maxY)
      {
      maxX = static_cast<int>(p2[0]);
      maxY = static_cast<int>(p2[1]);
      }
    if (p3[1] > maxY)
      {
      maxX = static_cast<int>(p3[0]);
      maxY = static_cast<int>(p3[1]);
      }
    if (p4[1] > maxY)
      {
      maxX = static_cast<int>(p4[0]);
      maxY = static_cast<int>(p4[1]);
      }
    int minX = VTK_INT_MAX, minY = VTK_INT_MAX;
    if (p1[1] < minY)
      {
      minX = static_cast<int>(p1[0]);
      minY = static_cast<int>(p1[1]);
      }
    if (p2[1] < minY)
      {
      minX = static_cast<int>(p2[0]);
      minY = static_cast<int>(p2[1]);
      }
    if (p3[1] < minY)
      {
      minX = static_cast<int>(p3[0]);
      minY = static_cast<int>(p3[1]);
      }
    if (p4[1] < minY)
      {
      minX = static_cast<int>(p4[0]);
      minY = static_cast<int>(p4[1]);
      }
    int textSize[2];
    this->TextMapper->GetSize(this->Renderer, textSize);
    if (this->ShowLabelAboveWidget)
      {
      this->TextActor->SetPosition(maxX - textSize[0]/2, maxY+9);
      }
    else
      {
      this->TextActor->SetPosition(minX - textSize[0]/2, minY-(textSize[1]+9));
      }

    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
char* vtkBiDimensionalRepresentation2D::GetLabelText()
{
  return this->TextMapper->GetInput();
}

//----------------------------------------------------------------------
double* vtkBiDimensionalRepresentation2D::GetLabelPosition()
{
  return this->TextActor->GetPosition();
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::GetLabelPosition(double pos[3])
{
  this->TextActor->GetPositionCoordinate()->GetValue(pos);
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::GetWorldLabelPosition(double pos[3])
{
  double viewportPos[3], worldPos[4];
  pos[0] = pos[1] = pos[2] = 0.0;
  if (!this->Renderer)
    {
    vtkErrorMacro("GetWorldLabelPosition: no renderer!");
    return;
  }
  this->TextActor->GetPositionCoordinate()->GetValue(viewportPos);
  this->Renderer->ViewportToNormalizedViewport(viewportPos[0], viewportPos[1]);
  this->Renderer->NormalizedViewportToView(viewportPos[0], viewportPos[1], viewportPos[2]);
  this->Renderer->SetViewPoint(viewportPos);
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint(worldPos);

  if (worldPos[3] != 0.0)
    {
    pos[0] = worldPos[0]/worldPos[3];
    pos[1] = worldPos[1]/worldPos[3];
    pos[2] = worldPos[2]/worldPos[3];
    }
  else
    {
    vtkErrorMacro("GetWorldLabelPosition: world position at index 3 is 0, not dividing by 0");
    }
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::ReleaseGraphicsResources(vtkWindow *w)
{
  this->LineActor->ReleaseGraphicsResources(w);
  this->TextActor->ReleaseGraphicsResources(w);
}


//----------------------------------------------------------------------
int vtkBiDimensionalRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  this->BuildRepresentation();

  int count = this->LineActor->RenderOverlay(viewport);
  if ( this->Line1Visibility )
    {
    count += this->TextActor->RenderOverlay(viewport);
    }
  return count;
}


//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::Highlight(int highlightOn)
{
  if ( highlightOn )
    {
    this->LineActor->SetProperty(this->SelectedLineProperty);
    }
  else
    {
    this->LineActor->SetProperty(this->LineProperty);
    }
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  if ( this->TextProperty )
    {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Property: (none)\n";
    }

  if ( this->LineProperty )
    {
    os << indent << "Line Property:\n";
    this->LineProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Line Property: (none)\n";
    }

  if ( this->SelectedLineProperty )
    {
    os << indent << "Selected Line Property:\n";
    this->SelectedLineProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Selected Line Property: (none)\n";
    }
}

