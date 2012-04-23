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

  this->RulerMode = 0;
  this->RulerDistance = 1.0;
  this->NumberOfRulerTicks = 5;
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
int vtkDistanceRepresentation::
ComputeInteractionState(int vtkNotUsed(X), int vtkNotUsed(Y), int vtkNotUsed(modify))
{
  if (this->Point1Representation == NULL || this->Point2Representation == NULL)
    {
    this->InteractionState = vtkDistanceRepresentation::Outside;
    return this->InteractionState;
    }

  int h1State = this->Point1Representation->GetInteractionState();
  int h2State = this->Point2Representation->GetInteractionState();
  if ( h1State == vtkHandleRepresentation::Nearby )
    {
    this->InteractionState = vtkDistanceRepresentation::NearP1;
    }
  else if ( h2State == vtkHandleRepresentation::Nearby )
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
  // Make sure that tolerance is consistent between handles and this representation
  if(this->Point1Representation)
    {
    this->Point1Representation->SetTolerance(this->Tolerance);
    }
  if(this->Point2Representation)
    {
    this->Point2Representation->SetTolerance(this->Tolerance);
    }
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

  os << indent << "Ruler Mode: "
     << (this->RulerMode ? "On" : "Off") <<"\n";
  os << indent << "Ruler Distance: " << this->GetRulerDistance() <<"\n";
  os << indent << "Number of Ruler Ticks: " << this->GetNumberOfRulerTicks() <<"\n";

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
