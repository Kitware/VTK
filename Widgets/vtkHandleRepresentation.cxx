/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHandleRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHandleRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkInteractorObserver.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"


vtkCxxRevisionMacro(vtkHandleRepresentation, "1.2");

//----------------------------------------------------------------------
vtkHandleRepresentation::vtkHandleRepresentation()
{
  // Positions are maintained via a vtkCoordinate
  this->DisplayPosition = vtkCoordinate::New();
  this->DisplayPosition->SetCoordinateSystemToDisplay();

  this->WorldPosition = vtkCoordinate::New();
  this->WorldPosition->SetCoordinateSystemToWorld();

  this->InteractionState = vtkHandleRepresentation::Outside;
  this->Tolerance = 15;
  this->ActiveRepresentation = 0;

  this->DisplayPositionTime.Modified();
  this->WorldPositionTime.Modified();
}

//----------------------------------------------------------------------
vtkHandleRepresentation::~vtkHandleRepresentation()
{
  this->DisplayPosition->Delete();
  this->WorldPosition->Delete();
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::SetDisplayPosition(double pos[3])
{
  this->DisplayPosition->SetValue(pos);
  if ( this->Renderer )
    {
    double *p = this->DisplayPosition->GetComputedWorldValue(this->Renderer);
    this->WorldPosition->SetValue(p[0], p[1], p[2]);
    }
  this->DisplayPositionTime.Modified();
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::GetDisplayPosition(double pos[3])
{
  // The world position maintains the position
  if ( this->Renderer && (this->WorldPositionTime > this->DisplayPositionTime) )
    {
    int *p = this->WorldPosition->GetComputedDisplayValue(this->Renderer);
    this->DisplayPosition->SetValue(p[0],p[1],0.0);
    }
  this->DisplayPosition->GetValue(pos);
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::SetWorldPosition(double pos[3])
{
  this->WorldPosition->SetValue(pos);
  this->DisplayPositionTime.Modified();
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::GetWorldPosition(double pos[3])
{
  this->WorldPosition->GetValue(pos);
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::SetRenderer(vtkRenderer *ren)
{
  this->DisplayPosition->SetViewport(ren);
  this->WorldPosition->SetViewport(ren);
  this->Superclass::SetRenderer(ren);
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::ShallowCopy(vtkProp *prop)
{
  vtkHandleRepresentation *rep = vtkHandleRepresentation::SafeDownCast(prop);
  if ( rep )
    {
    this->SetTolerance(rep->GetTolerance());
    this->SetActiveRepresentation(rep->GetActiveRepresentation());
    this->SetConstrained(rep->GetConstrained());
    }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "Active Representation: " 
     << (this->ActiveRepresentation ? "On" : "Off") << "\n";
}
