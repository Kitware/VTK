/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextRepresentation.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkTextRepresentation, "1.3");
vtkStandardNewMacro(vtkTextRepresentation);

//-------------------------------------------------------------------------
vtkTextRepresentation::vtkTextRepresentation()
{
  this->TextActor = vtkTextActor::New();
  this->TextActor->ScaledTextOn();
  this->TextActor->SetMinimumSize(1,1);
  this->TextActor->SetMaximumLineHeight(1.0);
  this->TextActor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->TextActor->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->TextActor->GetPosition2Coordinate()->SetReferenceCoordinate(0);
  this->TextActor->GetTextProperty()->SetJustificationToCentered();
  this->TextActor->GetTextProperty()->SetVerticalJustificationToCentered();
 
  this->ShowBorder = vtkBorderRepresentation::BORDER_ACTIVE;
}

//-------------------------------------------------------------------------
vtkTextRepresentation::~vtkTextRepresentation()
{
  this->SetTextActor(0);
}

//-------------------------------------------------------------------------
void vtkTextRepresentation::SetTextActor(vtkTextActor *textActor)
{
  if ( textActor != this->TextActor )
    {
    if ( this->TextActor )
      {
      this->TextActor->Delete();
      }
    this->TextActor = textActor;
    if ( this->TextActor )
      {
      this->TextActor->Register(this);
      this->TextActor->ScaledTextOn();
      this->TextActor->SetMinimumSize(1,1);
      this->TextActor->SetMaximumLineHeight(1.0);
      this->TextActor->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
      this->TextActor->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
      this->TextActor->GetPosition2Coordinate()->SetReferenceCoordinate(0);
      this->TextActor->GetTextProperty()->SetJustificationToCentered();
      this->TextActor->GetTextProperty()->SetVerticalJustificationToCentered();
      }
    this->Modified();
    }
}

//-------------------------------------------------------------------------
void vtkTextRepresentation::BuildRepresentation()
{
  // Ask the superclass the size and set the text
  int *pos1 = this->PositionCoordinate->GetComputedDisplayValue(this->Renderer);
  int *pos2 = this->Position2Coordinate->GetComputedDisplayValue(this->Renderer);

  if ( this->TextActor )
    {
    this->TextActor->GetPositionCoordinate()->SetValue(pos1[0],pos1[1]);
    this->TextActor->GetPosition2Coordinate()->SetValue(pos2[0],pos2[1]);
    }

  // Note that the transform is updated by the superclass
  this->Superclass::BuildRepresentation();
}

//-------------------------------------------------------------------------
void vtkTextRepresentation::GetActors2D(vtkPropCollection *pc)
{
  pc->AddItem(this->TextActor);
  this->Superclass::GetActors2D(pc);
}

//-------------------------------------------------------------------------
void vtkTextRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->TextActor->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int vtkTextRepresentation::RenderOverlay(vtkViewport *w)
{
  int count = this->Superclass::RenderOverlay(w);
  count += this->TextActor->RenderOverlay(w);
  return count;
}

//-------------------------------------------------------------------------
int vtkTextRepresentation::RenderOpaqueGeometry(vtkViewport *w)
{
  int count = this->Superclass::RenderOpaqueGeometry(w);
  count += this->TextActor->RenderOpaqueGeometry(w);
  return count;
}

//-------------------------------------------------------------------------
int vtkTextRepresentation::RenderTranslucentGeometry(vtkViewport *w)
{
  int count = this->Superclass::RenderTranslucentGeometry(w);
  count += this->TextActor->RenderTranslucentGeometry(w);
  return count;
}


//-------------------------------------------------------------------------
void vtkTextRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Text Actor: " << this->TextActor << "\n";
  
}
