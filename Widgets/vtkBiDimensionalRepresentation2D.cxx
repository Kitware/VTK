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

vtkCxxRevisionMacro(vtkBiDimensionalRepresentation2D, "1.7");
vtkStandardNewMacro(vtkBiDimensionalRepresentation2D);


//----------------------------------------------------------------------
vtkBiDimensionalRepresentation2D::vtkBiDimensionalRepresentation2D()
{
  // By default, use one of these handles
  this->HandleRepresentation  = vtkPointHandleRepresentation2D::New();
  this->Point1Representation = NULL;
  this->Point2Representation = NULL;
  this->Point3Representation = NULL;
  this->Point4Representation = NULL;
  this->InstantiateHandleRepresentation();

  this->Modifier = 0;

  this->Tolerance = 5;
  this->Placed = 0;
  
  this->Line1Visibility = 1;
  this->Line2Visibility = 1;
  
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
  this->L1TextMapper = vtkTextMapper::New();
  this->L1TextMapper->SetTextProperty(this->TextProperty);
  this->L1TextMapper->SetInput("0.0");
  this->L1TextActor = vtkActor2D::New();
  this->L1TextActor->SetMapper(this->L1TextMapper);
  this->L1TextActor->VisibilityOff();

  this->L2TextMapper = vtkTextMapper::New();
  this->L2TextMapper->SetInput("0.0");
  this->L2TextMapper->SetTextProperty(this->TextProperty);
  this->L2TextActor = vtkActor2D::New();
  this->L2TextActor->SetMapper(this->L2TextMapper);
  this->L2TextActor->VisibilityOff();
}

//----------------------------------------------------------------------
vtkBiDimensionalRepresentation2D::~vtkBiDimensionalRepresentation2D()
{
  if ( this->HandleRepresentation )
    {
    this->HandleRepresentation->Delete();
    }
  if ( this->Point1Representation )
    {
    this->Point1Representation->Delete();
    }
  if ( this->Point2Representation )
    {
    this->Point2Representation->Delete();
    }
  if ( this->Point3Representation )
    {
    this->Point3Representation->Delete();
    }
  if ( this->Point4Representation )
    {
    this->Point4Representation->Delete();
    }

  this->LineCells->Delete();
  this->LinePoints->Delete();
  this->LinePolyData->Delete();
  this->LineMapper->Delete();
  this->LineProperty->Delete();
  this->LineActor->Delete();
  this->SelectedLineProperty->Delete();
  this->TextProperty->Delete();
  this->L1TextMapper->Delete();
  this->L2TextMapper->Delete();
  this->L1TextActor->Delete();
  this->L2TextActor->Delete();
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D
::SetHandleRepresentation(vtkHandleRepresentation *handle)
{
  if ( handle == NULL || handle == this->HandleRepresentation )
    {
    return;
    }
  
  this->Modified();
  this->HandleRepresentation->Delete();
  this->HandleRepresentation = handle;
  this->HandleRepresentation->Register(this);
  
  this->Point1Representation->Delete();
  this->Point2Representation->Delete();
  this->Point3Representation->Delete();
  this->Point4Representation->Delete();

  this->Point1Representation = NULL;
  this->Point2Representation = NULL;
  this->Point3Representation = NULL;
  this->Point4Representation = NULL;

  this->InstantiateHandleRepresentation();
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::GetPoint1WorldPosition(double pos[3])
{
  this->Point1Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::GetPoint2WorldPosition(double pos[3])
{
  this->Point2Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::GetPoint3WorldPosition(double pos[3])
{
  this->Point3Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::GetPoint4WorldPosition(double pos[3])
{
  this->Point4Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::SetPoint1DisplayPosition(double x[3])
{
  this->Point1Representation->SetDisplayPosition(x);
  double p[3];
  this->Point1Representation->GetWorldPosition(p);
  this->Point1Representation->SetWorldPosition(p);
  this->Modified();
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::SetPoint2DisplayPosition(double x[3])
{
  this->Point2Representation->SetDisplayPosition(x);
  double p[3];
  this->Point2Representation->GetWorldPosition(p);
  this->Point2Representation->SetWorldPosition(p);
  this->Modified();
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::SetPoint3DisplayPosition(double x[3])
{
  this->Point3Representation->SetDisplayPosition(x);
  double p[3];
  this->Point3Representation->GetWorldPosition(p);
  this->Point3Representation->SetWorldPosition(p);
  this->Modified();
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::SetPoint4DisplayPosition(double x[3])
{
  this->Point4Representation->SetDisplayPosition(x);
  double p[3];
  this->Point4Representation->GetWorldPosition(p);
  this->Point4Representation->SetWorldPosition(p);
  this->Modified();
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::GetPoint1DisplayPosition(double pos[3])
{
  this->Point1Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::GetPoint2DisplayPosition(double pos[3])
{
  this->Point2Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::GetPoint3DisplayPosition(double pos[3])
{
  this->Point3Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::GetPoint4DisplayPosition(double pos[3])
{
  this->Point4Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

  
//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::InstantiateHandleRepresentation()
{
  if ( ! this->Point1Representation )
    {
    this->Point1Representation = this->HandleRepresentation->NewInstance();
    this->Point1Representation->ShallowCopy(this->HandleRepresentation);
    }
  
  if ( ! this->Point2Representation )
    {
    this->Point2Representation = this->HandleRepresentation->NewInstance();
    this->Point2Representation->ShallowCopy(this->HandleRepresentation);
    }

  if ( ! this->Point3Representation )
    {
    this->Point3Representation = this->HandleRepresentation->NewInstance();
    this->Point3Representation->ShallowCopy(this->HandleRepresentation);
    }
  
  if ( ! this->Point4Representation )
    {
    this->Point4Representation = this->HandleRepresentation->NewInstance();
    this->Point4Representation->ShallowCopy(this->HandleRepresentation);
    }
}
  
//----------------------------------------------------------------------
int vtkBiDimensionalRepresentation2D::ComputeInteractionState(int X, int Y, int modify)
{
  this->Modifier = modify;

  // See if we are near one of the end points or outside
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
  if ( vtkMath::Distance2BetweenPoints(xyz,p1) <= tol2 )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::NearP1;
    }
  else if ( vtkMath::Distance2BetweenPoints(xyz,p2) <= tol2 )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::NearP2;
    }
  else if ( vtkMath::Distance2BetweenPoints(xyz,p3) <= tol2 )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::NearP3;
    }
  else if ( vtkMath::Distance2BetweenPoints(xyz,p4) <= tol2 )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::NearP4;
    }
  else if ( vtkLine::DistanceToLine(xyz,p1,p2,t,closest) <= tol2 )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::OnL1;
    }
  else if ( vtkLine::DistanceToLine(xyz,p3,p4,t,closest) <= tol2 )
    {
    this->InteractionState = vtkBiDimensionalRepresentation2D::OnL2;
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
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetPoint2DisplayPosition(pos);
  this->Modified();
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
  this->GetPoint1DisplayPosition(p1);
  this->GetPoint2DisplayPosition(p2);
  slope1[0] = p2[0] - p1[0];
  slope1[1] = p2[1] - p1[1];
  slope2[0] = -slope1[1];
  slope2[1] =  slope1[0];
  slope2[2] = 0.0;
  vtkMath::Normalize(slope2);
  
  // The current position of P3 is constrained to lie along Line1. Also,
  // P4 is placed on the opposite side of Line1.
  double p[3], t, closest[3];
  p[0] = e[0];
  p[1] = e[1];
  p[2] = p3[2] = p4[2] = 0.0;
  double dist = sqrt(vtkLine::DistanceToLine(p,p1,p2,t,closest));
  
  // Set the positions of P3 and P4.
  p3[0] = closest[0] + dist*slope2[0];
  p3[1] = closest[1] + dist*slope2[1];
  this->SetPoint3DisplayPosition(p3);

  p4[0] = closest[0] - dist*slope2[0];
  p4[1] = closest[1] - dist*slope2[1];
  this->SetPoint4DisplayPosition(p4);
  
  this->Modified();
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::StartWidgetManipulation(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  this->GetPoint1DisplayPosition(this->P1);
  this->GetPoint2DisplayPosition(this->P2);
  this->GetPoint3DisplayPosition(this->P3);
  this->GetPoint4DisplayPosition(this->P4);
  this->P1[2] = this->P2[2] = this->P3[2] = this->P4[2] = 0.0;
  for (int i=0; i<3; i++)
    {
    this->P21[i] = this->P2[i] - this->P1[i];
    this->P43[i] = this->P4[i] - this->P3[i];
    }

  vtkLine::Intersection(this->P1,this->P2,this->P3,this->P4,this->T21,this->T43);
}

//----------------------------------------------------------------------
// This method is tricky because it is constrained by Line1 and Line2.
// (This method is invoked after all four points have been placed.)
void vtkBiDimensionalRepresentation2D::WidgetInteraction(double e[2])
{
  double pos[3], t, closest[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;

  // Depending on the state, different motions are allowed.
  if ( this->InteractionState == Outside )
    {
    return;
    }
  else if ( this->Modifier ) //rotate the representation
    {
    // compute rotation angle and center of rotation
    double c[3], sc[3], ec[3], p1c[3], p2c[3], p3c[3], p4c[3];
    double p1[3], p2[3], p3[3], p4[3];
    for (int i=0; i<3; i++)
      {
      c[i] = ((this->P1[i] + this->T21*this->P21[i]) + (this->P3[i] + this->T43*this->P43[i]))/2.0;
      sc[i] = this->StartEventPosition[i] - c[i];
      ec[i] = pos[i] - c[i];
      p1c[i] = this->P1[i] - c[i];
      p2c[i] = this->P2[i] - c[i];
      p3c[i] = this->P3[i] - c[i];
      p4c[i] = this->P4[i] - c[i];
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
    p1[0] = c[0] + r1*cos(theta+theta1);
    p1[1] = c[1] + r1*sin(theta+theta1);
    p2[0] = c[0] + r2*cos(theta+theta2);
    p2[1] = c[1] + r2*sin(theta+theta2);
    p3[0] = c[0] + r3*cos(theta+theta3);
    p3[1] = c[1] + r3*sin(theta+theta3);
    p4[0] = c[0] + r4*cos(theta+theta4);
    p4[1] = c[1] + r4*sin(theta+theta4);
    p1[2] = p2[2] = p3[2] = p4[2] = 0.0;
    this->SetPoint1DisplayPosition(p1);
    this->SetPoint2DisplayPosition(p2);
    this->SetPoint3DisplayPosition(p3);
    this->SetPoint4DisplayPosition(p4);
    }
  else if ( this->InteractionState == NearP1 )
    {
    double p1[3];
    vtkLine::DistanceToLine(pos,this->P1,this->P2,t,closest);
    t = (t > this->T21 ? this->T21 : t);
    p1[0] = this->P1[0] + t*this->P21[0];
    p1[1] = this->P1[1] + t*this->P21[1];
    p1[2] = 0.0;
    
    // Set the positions of P1
    this->SetPoint1DisplayPosition(p1);
    }
  else if ( this->InteractionState == NearP2 )
    {
    double p2[3];
    vtkLine::DistanceToLine(pos,this->P1,this->P2,t,closest);
    t = (t < this->T21 ? this->T21 : t);
    p2[0] = this->P1[0] + t*this->P21[0];
    p2[1] = this->P1[1] + t*this->P21[1];
    p2[2] = 0.0;

    // Set the position of P2
    this->SetPoint2DisplayPosition(p2);
    }
  else if ( this->InteractionState == NearP3 )
    {
    double p3[3];
    vtkLine::DistanceToLine(pos,this->P3,this->P4,t,closest);
    t = (t > this->T43 ? this->T43 : t);
    p3[0] = this->P3[0] + t*this->P43[0];
    p3[1] = this->P3[1] + t*this->P43[1];
    p3[2] = 0.0;

    // Set the position of P3 
    this->SetPoint3DisplayPosition(p3);
    }
  else if ( this->InteractionState == NearP4 )
    {
    double p4[3];
    vtkLine::DistanceToLine(pos,this->P3,this->P4,t,closest);
    t = (t < this->T43 ? this->T43 : t);
    p4[0] = this->P3[0] + t*this->P43[0];
    p4[1] = this->P3[1] + t*this->P43[1];
    p4[2] = 0.0;

    // Set the position of P4 
    this->SetPoint4DisplayPosition(p4);
    }
  else if ( this->InteractionState == OnL1 )
    {
    double p1[3], p2[3];
    vtkLine::DistanceToLine(pos,this->P3,this->P4,t,closest);
    t = ( t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t) );
    p1[0] = this->P1[0] + (t-this->T43)*this->P43[0];
    p1[1] = this->P1[1] + (t-this->T43)*this->P43[1];
    p2[0] = this->P2[0] + (t-this->T43)*this->P43[0];
    p2[1] = this->P2[1] + (t-this->T43)*this->P43[1];
    p1[2] = p2[2] = 0.0;

    // Set the positions of P1 and P2.
    this->SetPoint1DisplayPosition(p1);
    this->SetPoint2DisplayPosition(p2);
    }
  else if ( this->InteractionState == OnL2 )
    {
    double p3[3], p4[3];
    vtkLine::DistanceToLine(pos,this->P1,this->P2,t,closest);
    t = ( t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t) );
    p3[0] = this->P3[0] + (t-this->T21)*this->P21[0];
    p3[1] = this->P3[1] + (t-this->T21)*this->P21[1];
    p4[0] = this->P4[0] + (t-this->T21)*this->P21[0];
    p4[1] = this->P4[1] + (t-this->T21)*this->P21[1];
    p3[2] = p4[2] = 0.0;

    // Set the positions of P3 and P4.
    this->SetPoint3DisplayPosition(p3);
    this->SetPoint4DisplayPosition(p4);
    }

  this->Modified();
}

//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime || 
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    this->BuildTime.Modified();

    this->Point1Representation->BuildRepresentation();
    this->Point2Representation->BuildRepresentation();
    this->Point3Representation->BuildRepresentation();
    this->Point4Representation->BuildRepresentation();

    // Now bring the lines up to date
    if ( ! this->Line1Visibility )
      {
      return;
      }

    char str[256];
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

    sprintf(str,"%0.3g",sqrt(vtkMath::Distance2BetweenPoints(wp1,wp2)));
    this->L1TextMapper->SetInput(str);
    this->L1TextActor->SetPosition(p2[0]+7,p2[1]+7);

    this->LineCells->Reset();
    this->LineCells->InsertNextCell(2);
    this->LineCells->InsertCellPoint(0);
    this->LineCells->InsertCellPoint(1);

    if ( this->Line2Visibility )
      {
      sprintf(str,"%0.3g",sqrt(vtkMath::Distance2BetweenPoints(wp3,wp4)));
      this->L2TextMapper->SetInput(str);
      this->L2TextActor->SetPosition(p4[0]+7,p4[1]+7);
      this->LineCells->InsertNextCell(2);
      this->LineCells->InsertCellPoint(2);
      this->LineCells->InsertCellPoint(3);
      }
  }
}

//----------------------------------------------------------------------
double vtkBiDimensionalRepresentation2D::GetLength1()
{
  double x1[3], x2[3];
  
  this->GetPoint1WorldPosition(x1);
  this->GetPoint2WorldPosition(x2);
  
  return sqrt(vtkMath::Distance2BetweenPoints(x1,x2));
}


//----------------------------------------------------------------------
double vtkBiDimensionalRepresentation2D::GetLength2()
{
  double x3[3], x4[3];
  
  this->GetPoint3WorldPosition(x3);
  this->GetPoint4WorldPosition(x4);
  
  return sqrt(vtkMath::Distance2BetweenPoints(x3,x4));
}


//----------------------------------------------------------------------
void vtkBiDimensionalRepresentation2D::ReleaseGraphicsResources(vtkWindow *w)
{
  this->LineActor->ReleaseGraphicsResources(w);
  this->L1TextActor->ReleaseGraphicsResources(w);
  this->L2TextActor->ReleaseGraphicsResources(w);
}


//----------------------------------------------------------------------
int vtkBiDimensionalRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  this->BuildRepresentation();

  int count = this->LineActor->RenderOverlay(viewport);
  if ( this->Line1Visibility )
    {
    count += this->L1TextActor->RenderOverlay(viewport);
    }
  if ( this->Line2Visibility )
    {
    count += this->L2TextActor->RenderOverlay(viewport);
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
  
  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "Length1: " << this->GetLength1() << "\n";
  os << indent << "Length2: " << this->GetLength2() << "\n";

  os << indent << "Line1 Visibility: " << (this->Line1Visibility ? "On\n" : "Off\n");
  os << indent << "Line2 Visibility: " << (this->Line2Visibility ? "On\n" : "Off\n");

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

