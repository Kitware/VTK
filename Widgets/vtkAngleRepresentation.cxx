/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAngleRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAngleRepresentation.h"
#include "vtkHandleRepresentation.h"
#include "vtkActor2D.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkCoordinate.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkTextProperty.h"
#include "vtkWindow.h"


vtkCxxSetObjectMacro(vtkAngleRepresentation,HandleRepresentation,vtkHandleRepresentation);


//----------------------------------------------------------------------
vtkAngleRepresentation::vtkAngleRepresentation()
{
  this->HandleRepresentation  = NULL;
  this->Point1Representation = NULL;
  this->CenterRepresentation = NULL;
  this->Point2Representation = NULL;

  this->Tolerance = 5;
  this->Placed = 0;
  
  this->Ray1Visibility = 1;
  this->Ray2Visibility = 1;
  this->ArcVisibility = 1;
  
  this->LabelFormat = new char[8]; 
  sprintf(this->LabelFormat,"%s","%-#6.3g");
}

//----------------------------------------------------------------------
vtkAngleRepresentation::~vtkAngleRepresentation()
{
  if ( this->HandleRepresentation )
    {
    this->HandleRepresentation->Delete();
    }
  if ( this->Point1Representation )
    {
    this->Point1Representation->Delete();
    }
  if ( this->CenterRepresentation )
    {
    this->CenterRepresentation->Delete();
    }
  if ( this->Point2Representation )
    {
    this->Point2Representation->Delete();
    }

  if (this->LabelFormat) 
    {
    delete [] this->LabelFormat;
    this->LabelFormat = NULL;
    }
}

  
//----------------------------------------------------------------------
void vtkAngleRepresentation::InstantiateHandleRepresentation()
{
  if ( ! this->Point1Representation )
    {
    this->Point1Representation = this->HandleRepresentation->NewInstance();
    this->Point1Representation->ShallowCopy(this->HandleRepresentation);
    }
  
  if ( ! this->CenterRepresentation )
    {
    this->CenterRepresentation = this->HandleRepresentation->NewInstance();
    this->CenterRepresentation->ShallowCopy(this->HandleRepresentation);
    }
  
  if ( ! this->Point2Representation )
    {
    this->Point2Representation = this->HandleRepresentation->NewInstance();
    this->Point2Representation->ShallowCopy(this->HandleRepresentation);
    }
}
  
//----------------------------------------------------------------------
int vtkAngleRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // See if we are near one of the end points or outside
  double pos1[3], pos2[3], center[3];
  this->GetPoint1DisplayPosition(pos1);
  this->GetCenterDisplayPosition(center);
  this->GetPoint2DisplayPosition(pos2);
  
  double p1[3], p2[3], c[3], xyz[3];
  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  p1[0] = static_cast<double>(pos1[0]);
  p1[1] = static_cast<double>(pos1[1]);
  c[0] = static_cast<double>(center[0]);
  c[1] = static_cast<double>(center[1]);
  p2[0] = static_cast<double>(pos2[0]);
  p2[1] = static_cast<double>(pos2[1]);
  xyz[2] = p1[2] = p2[2] = c[2] = 0.0;

  double tol2 = this->Tolerance*this->Tolerance;
  if ( vtkMath::Distance2BetweenPoints(xyz,p1) <= tol2 )
    {
    this->InteractionState = vtkAngleRepresentation::NearP1;
    }
  else if ( vtkMath::Distance2BetweenPoints(xyz,c) <= tol2 )
    {
    this->InteractionState = vtkAngleRepresentation::NearCenter;
    }
  else if ( vtkMath::Distance2BetweenPoints(xyz,p2) <= tol2 )
    {
    this->InteractionState = vtkAngleRepresentation::NearP2;
    }
  else 
    {
    this->InteractionState = vtkAngleRepresentation::Outside;
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkAngleRepresentation::StartWidgetInteraction(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetPoint1DisplayPosition(pos);
  this->SetCenterDisplayPosition(pos);
  this->SetPoint2DisplayPosition(pos);
}

//----------------------------------------------------------------------
void vtkAngleRepresentation::CenterWidgetInteraction(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetCenterDisplayPosition(pos);
  this->SetPoint2DisplayPosition(pos);
}

//----------------------------------------------------------------------
void vtkAngleRepresentation::WidgetInteraction(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetPoint2DisplayPosition(pos);
}

//----------------------------------------------------------------------
void vtkAngleRepresentation::BuildRepresentation()
{
  // We don't worry about mtime 'cause the subclass deals with that
  // Make sure the handles are up to date
  this->Point1Representation->BuildRepresentation();
  this->CenterRepresentation->BuildRepresentation();
  this->Point2Representation->BuildRepresentation();
}

//----------------------------------------------------------------------
void vtkAngleRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Angle: " << this->GetAngle() << "\n";
  os << indent << "Tolerance: " << this->Tolerance <<"\n";
  os << indent << "Ray1 Visibility: " << (this->Ray1Visibility ? "On\n" : "Off\n");
  os << indent << "Ray2 Visibility: " << (this->Ray2Visibility ? "On\n" : "Off\n");
  os << indent << "Arc Visibility: " << (this->ArcVisibility ? "On\n" : "Off\n");
  os << indent << "Handle Representation: " << this->HandleRepresentation << "\n";

  os << indent << "Label Format: ";
  if ( this->LabelFormat )
    {
    os << this->LabelFormat << "\n";
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Point1 Representation: ";
  if ( this->Point1Representation )
    {
    this->Point1Representation->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Center Representation: ";
  if ( this->CenterRepresentation )
    {
    this->CenterRepresentation->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Point2 Representation: ";
  if ( this->Point2Representation )
    {
    this->Point2Representation->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}
