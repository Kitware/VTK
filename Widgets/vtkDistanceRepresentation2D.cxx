/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistanceRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDistanceRepresentation2D.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkAxisActor2D.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkLineSource.h"
#include "vtkProperty2D.h"
#include "vtkCoordinate.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkBox.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkTextProperty.h"
#include "vtkWindow.h"

vtkCxxRevisionMacro(vtkDistanceRepresentation2D, "1.3");
vtkStandardNewMacro(vtkDistanceRepresentation2D);

//----------------------------------------------------------------------
vtkDistanceRepresentation2D::vtkDistanceRepresentation2D()
{
  this->AxisProperty = vtkProperty2D::New();
  this->AxisProperty->SetColor(0,1,0);

  this->AxisActor = vtkAxisActor2D::New();
  this->AxisActor->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->AxisActor->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->AxisActor->SetNumberOfLabels(5);
  this->AxisActor->LabelVisibilityOff();
  this->AxisActor->AdjustLabelsOff();
  this->AxisActor->SetProperty(this->AxisProperty);
  this->AxisActor->SetTitle("Distance");
  this->AxisActor->GetTitleTextProperty()->ShadowOff();
  this->AxisActor->GetTitleTextProperty()->SetColor(0,1,0);
  
  this->Distance = 0.0;
}

//----------------------------------------------------------------------
vtkDistanceRepresentation2D::~vtkDistanceRepresentation2D()
{
  this->AxisProperty->Delete();
  this->AxisActor->Delete();
}

  
//----------------------------------------------------------------------
void vtkDistanceRepresentation2D::GetPoint1WorldPosition(double pos[3])
{
  this->Point1Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation2D::GetPoint2WorldPosition(double pos[3])
{
  this->Point2Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation2D::SetPoint1DisplayPosition(double x[3])
{
  this->Point1Representation->SetDisplayPosition(x);
  double p[3];
  this->Point1Representation->GetWorldPosition(p);
  this->Point1Representation->SetWorldPosition(p);
  this->AxisActor->GetPoint1Coordinate()->SetValue(p);
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation2D::SetPoint2DisplayPosition(double x[3])
{
  this->Point2Representation->SetDisplayPosition(x);
  double p[3];
  this->Point2Representation->GetWorldPosition(p);
  this->Point2Representation->SetWorldPosition(p);
  this->AxisActor->GetPoint2Coordinate()->SetValue(p);
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation2D::GetPoint1DisplayPosition(double pos[3])
{
  this->Point1Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation2D::GetPoint2DisplayPosition(double pos[3])
{
  this->Point2Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
vtkAxisActor2D *vtkDistanceRepresentation2D::GetAxis()
{
  return this->AxisActor;
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation2D::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime || 
       this->Point1Representation->GetMTime() > this->BuildTime ||
       this->Point2Representation->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    this->Superclass::BuildRepresentation();
    
    // Okay, compute the distance and set the label
    double p1[3], p2[3];
    this->Point1Representation->GetWorldPosition(p1);
    this->Point2Representation->GetWorldPosition(p2);
    this->Distance = sqrt(vtkMath::Distance2BetweenPoints(p1,p2));
    char string[512];
    sprintf(string, this->LabelFormat, this->Distance);
    this->AxisActor->SetTitle(string);
    
    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation2D::ReleaseGraphicsResources(vtkWindow *w)
{
  this->AxisActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int vtkDistanceRepresentation2D::RenderOverlay(vtkViewport *v)
{
  this->BuildRepresentation();
  
  if ( this->AxisActor->GetVisibility() )
    {
    return this->AxisActor->RenderOverlay(v);
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------
int vtkDistanceRepresentation2D::RenderOpaqueGeometry(vtkViewport *v)
{
  this->BuildRepresentation();
  
  if ( this->AxisActor->GetVisibility() )
    {
    return this->AxisActor->RenderOpaqueGeometry(v);
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}
