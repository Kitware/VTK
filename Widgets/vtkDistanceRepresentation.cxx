/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDistanceRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDistanceRepresentation.h"
#include "vtkHandleRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkBox.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkWindow.h"

vtkCxxSetObjectMacro(vtkDistanceRepresentation,HandleRepresentation,vtkHandleRepresentation);


//----------------------------------------------------------------------
vtkDistanceRepresentation::vtkDistanceRepresentation()
{
  this->HandleRepresentation  = NULL;
  this->Point1Representation = NULL;
  this->Point2Representation = NULL;

  this->Tolerance = 5;
  this->Placed = 0;
  
  this->LabelFormat = new char[8];
  sprintf(this->LabelFormat,"%s","%-#6.3g");
}

//----------------------------------------------------------------------
vtkDistanceRepresentation::~vtkDistanceRepresentation()
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

  if (this->LabelFormat) 
    {
    delete [] this->LabelFormat;
    this->LabelFormat = NULL;
    }
}

  
//----------------------------------------------------------------------
void vtkDistanceRepresentation::InstantiateHandleRepresentation()
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
}
  

//----------------------------------------------------------------------
void vtkDistanceRepresentation::GetPoint1WorldPosition(double pos[3])
{
  this->Point1Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation::GetPoint2WorldPosition(double pos[3])
{
  this->Point2Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
int vtkDistanceRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // See if we are near one of the end points or outside
  double pos1[3], pos2[3];
  this->GetPoint1DisplayPosition(pos1);
  this->GetPoint2DisplayPosition(pos2);
  
  double p1[3], p2[3], xyz[3];
  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  p1[0] = static_cast<double>(pos1[0]);
  p1[1] = static_cast<double>(pos1[1]);
  p2[0] = static_cast<double>(pos2[0]);
  p2[1] = static_cast<double>(pos2[1]);
  xyz[2] = p1[2] = p2[2] = 0.0;

  double tol2 = this->Tolerance*this->Tolerance;
  if ( vtkMath::Distance2BetweenPoints(xyz,p1) <= tol2 )
    {
    this->InteractionState = vtkDistanceRepresentation::NearP1;
    }
  else if ( vtkMath::Distance2BetweenPoints(xyz,p2) <= tol2 )
    {
    this->InteractionState = vtkDistanceRepresentation::NearP2;
    }
  else 
    {
    this->InteractionState = vtkDistanceRepresentation::Outside;
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation::StartWidgetInteraction(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetPoint1DisplayPosition(pos);
  this->SetPoint2DisplayPosition(pos);
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation::WidgetInteraction(double e[2])
{
  double pos[3];
  pos[0] = e[0];
  pos[1] = e[1];
  pos[2] = 0.0;
  this->SetPoint2DisplayPosition(pos);
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation::BuildRepresentation()
{
  // We don't worry about mtime 'cause the subclass deals with that
  // Make sure the handles are up to date
  this->Point1Representation->BuildRepresentation();
  this->Point2Representation->BuildRepresentation();
}

//----------------------------------------------------------------------
void vtkDistanceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Distance: " << this->GetDistance() <<"\n";
  os << indent << "Tolerance: " << this->Tolerance <<"\n";
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
